#include "EgnisBlockoutImporter.h"
#include "EgnisBlockoutSettings.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DesktopPlatformModule.h"
#include "Dom/JsonObject.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDesktopPlatform.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "EgnisBlockout"

namespace EgnisBlockout
{
namespace
{
	struct FParsedFile
	{
		TArray<TSharedPtr<FJsonObject>> Levels;
		int32 Size = 0;
		int32 Zone = 0;
	};

	using FKeyGrid = TArray<TArray<FString>>;

	bool ParseFile(const FString& Json, FParsedFile& Out, FString& OutError)
	{
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			OutError = TEXT("El texto no es un JSON válido. Cópialo entero desde Egnis Blockout (Export -> Ver JSON de todos -> Copy text).");
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* Levels = nullptr;
		if (Root->TryGetArrayField(TEXT("levels"), Levels))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Levels)
			{
				if (Value.IsValid() && Value->Type == EJson::Object)
				{
					Out.Levels.Add(Value->AsObject());
				}
			}
		}
		else if (Root->HasField(TEXT("grid")))
		{
			Out.Levels.Add(Root);
		}

		if (Out.Levels.Num() == 0)
		{
			OutError = TEXT("El JSON no tiene ningún nivel (le falta \"levels\" o \"grid\").");
			return false;
		}

		Root->TryGetNumberField(TEXT("size"), Out.Size);

		FString ZoneText;
		if (Root->TryGetStringField(TEXT("zone"), ZoneText))
		{
			FString Digits;
			for (const TCHAR Char : ZoneText)
			{
				if (FChar::IsDigit(Char))
				{
					Digits.AppendChar(Char);
				}
			}
			Out.Zone = Digits.IsEmpty() ? 0 : FCString::Atoi(*Digits);
		}
		return true;
	}

	// Acepta la cuadrícula como filas (["WALL", ".", ...]) o como una lista plana de Size x Size claves
	bool ReadKeyGrid(const FJsonObject& Level, const TCHAR* Field, int32 Size, FKeyGrid& OutRows)
	{
		OutRows.Reset();
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Level.TryGetArrayField(Field, Values) || Values->Num() == 0)
		{
			return false;
		}

		const auto KeyOf = [](const TSharedPtr<FJsonValue>& Value)
		{
			return Value.IsValid() && Value->Type == EJson::String ? Value->AsString().TrimStartAndEnd().ToUpper() : FString();
		};

		if ((*Values)[0]->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& RowValue : *Values)
			{
				TArray<FString>& Row = OutRows.AddDefaulted_GetRef();
				if (RowValue.IsValid() && RowValue->Type == EJson::Array)
				{
					for (const TSharedPtr<FJsonValue>& Cell : RowValue->AsArray())
					{
						Row.Add(KeyOf(Cell));
					}
				}
			}
			return true;
		}

		const int32 Side = Size > 0 ? Size : FMath::RoundToInt(FMath::Sqrt(static_cast<float>(Values->Num())));
		if (Side <= 0 || Side * Side != Values->Num())
		{
			return false;
		}
		for (int32 Y = 0; Y < Side; ++Y)
		{
			TArray<FString>& Row = OutRows.AddDefaulted_GetRef();
			for (int32 X = 0; X < Side; ++X)
			{
				Row.Add(KeyOf((*Values)[Y * Side + X]));
			}
		}
		return true;
	}

	bool IsEmptyKey(const FString& Key)
	{
		return Key.IsEmpty() || Key == TEXT(".");
	}

	// Un nivel sin nada dibujado es una plantilla de la herramienta: "importar todos" no crea asset para él
	bool IsBlankLevel(const FJsonObject& Level, int32 Size)
	{
		const TArray<FString>& Ignored = GetDefault<UEgnisBlockoutSettings>()->IgnoredKeys;
		for (const TCHAR* Field : { TEXT("grid"), TEXT("objectives") })
		{
			FKeyGrid Rows;
			ReadKeyGrid(Level, Field, Size, Rows);
			for (const TArray<FString>& Row : Rows)
			{
				for (const FString& Key : Row)
				{
					if (!IsEmptyKey(Key) && !Ignored.Contains(Key))
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	FString GetStringField(const FJsonObject& Object, const TCHAR* Field)
	{
		FString Value;
		Object.TryGetStringField(Field, Value);
		return Value;
	}

	bool ReadDifficulty(const FJsonObject& Level, ELevelDifficulty& OutDifficulty)
	{
		const FString Name = GetStringField(Level, TEXT("difficulty")).Replace(TEXT(" "), TEXT(""));
		const UEnum* Enum = StaticEnum<ELevelDifficulty>();
		if (!Name.IsEmpty())
		{
			for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
			{
				if (Enum->GetNameStringByIndex(Index).Equals(Name, ESearchCase::IgnoreCase))
				{
					OutDifficulty = static_cast<ELevelDifficulty>(Enum->GetValueByIndex(Index));
					return true;
				}
			}
		}

		int32 Tier = 0;
		if (Level.TryGetNumberField(TEXT("tier"), Tier) && Tier >= 1 && Tier < Enum->NumEnums())
		{
			OutDifficulty = static_cast<ELevelDifficulty>(Tier - 1);
			return true;
		}
		return false;
	}

	// Vuelca un nivel del JSON en el asset. Los avisos no impiden importar (claves sin traducción, escolta a medias...)
	bool ApplyLevel(ULevelDataAsset* Asset, const FJsonObject& Level, int32 Size, const FString& SourceFile, TArray<FString>& OutWarnings, FString& OutError)
	{
		FKeyGrid Grid;
		if (!ReadKeyGrid(Level, TEXT("grid"), Size, Grid))
		{
			OutError = TEXT("El nivel no tiene una cuadrícula válida (\"grid\").");
			return false;
		}
		FKeyGrid Objectives;
		ReadKeyGrid(Level, TEXT("objectives"), Size, Objectives);

		const UEgnisBlockoutSettings* Settings = GetDefault<UEgnisBlockoutSettings>();

		int32 Width = 0;
		for (const TArray<FString>& Row : Grid)
		{
			Width = FMath::Max(Width, Row.Num());
		}

		Asset->Modify();
		Asset->Width = FMath::Max(Width, 1);
		Asset->Height = FMath::Max(Grid.Num(), 1);
		Asset->GridRows.Reset();
		Asset->ResizeGrid();
		Asset->EnemiesToSpawn.Reset();

		TMap<FString, int32> UnknownKeys;
		TSet<FString> MissingClasses;
		for (int32 Y = 0; Y < Grid.Num(); ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const FString Key = Grid[Y].IsValidIndex(X) ? Grid[Y][X] : FString();
				ETileType Type = ETileType::Normal;

				if (IsEmptyKey(Key) || Settings->IgnoredKeys.Contains(Key))
				{
				}
				else if (const ETileType* FoundType = Settings->TileTypes.Find(Key))
				{
					Type = *FoundType;
				}
				else if (const TSoftClassPtr<AActor>* EnemyClass = Settings->EnemyClasses.Find(Key))
				{
					FEnemySpawnInfo Enemy;
					Enemy.EnemyClass = EnemyClass->LoadSynchronous();
					Enemy.SpawnCoord = FTileCoord(X, Y);
					if (!Enemy.EnemyClass)
					{
						MissingClasses.Add(FString::Printf(TEXT("%s (%s)"), *Key, *EnemyClass->ToString()));
					}
					Asset->EnemiesToSpawn.Add(Enemy);
				}
				else
				{
					UnknownKeys.FindOrAdd(Key)++;
				}

				Asset->GridRows[Y].Columns[X] = Type;
			}
		}

		FSecondaryObjectives NewObjectives;
		bool bHasStart = false;
		bool bHasEnd = false;
		for (int32 Y = 0; Y < Objectives.Num() && Y < Asset->Height; ++Y)
		{
			for (int32 X = 0; X < Objectives[Y].Num() && X < Asset->Width; ++X)
			{
				const FString& Key = Objectives[Y][X];
				const FTileCoord Tile(X, Y);
				if (IsEmptyKey(Key))
				{
					continue;
				}
				if (Key == TEXT("A"))
				{
					NewObjectives.EscortStart = Tile;
					bHasStart = true;
				}
				else if (Key == TEXT("B"))
				{
					NewObjectives.EscortEnd = Tile;
					bHasEnd = true;
				}
				else if (Key == TEXT("PROT"))
				{
					NewObjectives.TilesToProtect.Add(Tile);
				}
				else if (Key == TEXT("DEST"))
				{
					NewObjectives.TilesToDestroy.Add(Tile);
				}
				else
				{
					UnknownKeys.FindOrAdd(FString::Printf(TEXT("objetivo %s"), *Key))++;
				}
			}
		}
		NewObjectives.bEscort = bHasStart && bHasEnd;
		if (bHasStart != bHasEnd)
		{
			OutWarnings.Add(FString::Printf(TEXT("La escolta solo tiene el punto %s: no se ha importado."), bHasStart ? TEXT("A") : TEXT("B")));
		}
		Asset->SecondaryObjectives = NewObjectives;

		ELevelDifficulty Difficulty;
		if (ReadDifficulty(Level, Difficulty))
		{
			Asset->Difficulty = Difficulty;
		}
		else
		{
			OutWarnings.Add(TEXT("No se ha reconocido la dificultad: se deja la que tenía."));
		}

		Asset->BlockoutLevelId = GetStringField(Level, TEXT("id"));
		Asset->BlockoutLevelName = GetStringField(Level, TEXT("name"));
		if (!SourceFile.IsEmpty())
		{
			Asset->BlockoutSourceFile = SourceFile;
		}

		for (const TPair<FString, int32>& Unknown : UnknownKeys)
		{
			OutWarnings.Add(FString::Printf(TEXT("%d casilla(s) con \"%s\" no tienen traducción en Project Settings > Egnis > Egnis Blockout: se han importado como Normal."),
				Unknown.Value, *Unknown.Key));
		}
		for (const FString& Missing : MissingClasses)
		{
			OutWarnings.Add(FString::Printf(TEXT("No existe el Blueprint del enemigo %s: el enemigo queda sin clase."), *Missing));
		}

		Asset->PostEditChange();
		Asset->MarkPackageDirty();
		return true;
	}

	TCHAR WithoutAccent(TCHAR Char)
	{
		static const FString From = TEXT("áàäâÁÀÄÂéèëêÉÈËÊíìïîÍÌÏÎóòöôÓÒÖÔúùüûÚÙÜÛñÑçÇ");
		static const FString To   = TEXT("aaaaAAAAeeeeEEEEiiiiIIIIooooOOOOuuuuUUUUnNcC");
		int32 Index = INDEX_NONE;
		return From.FindChar(Char, Index) ? To[Index] : Char;
	}

	FString SanitizeName(const FString& Name)
	{
		FString Result;
		bool bLastUnderscore = false;
		for (const TCHAR Original : Name)
		{
			const TCHAR Char = WithoutAccent(Original);
			const bool bValid = (Char >= 'a' && Char <= 'z') || (Char >= 'A' && Char <= 'Z') || (Char >= '0' && Char <= '9');
			if (bValid)
			{
				Result.AppendChar(Char);
				bLastUnderscore = false;
			}
			else if (!bLastUnderscore && !Result.IsEmpty())
			{
				Result.AppendChar('_');
				bLastUnderscore = true;
			}
		}
		Result.RemoveFromEnd(TEXT("_"));
		return Result.IsEmpty() ? TEXT("Level") : Result;
	}

	TSharedPtr<FJsonObject> PickLevelForAsset(const FParsedFile& File, const ULevelDataAsset* Asset, FString& OutError)
	{
		if (!Asset->BlockoutLevelId.IsEmpty())
		{
			for (const TSharedPtr<FJsonObject>& Level : File.Levels)
			{
				if (GetStringField(*Level, TEXT("id")) == Asset->BlockoutLevelId)
				{
					return Level;
				}
			}
		}

		if (File.Levels.Num() == 1)
		{
			return File.Levels[0];
		}

		const FString Prefix = GetDefault<UEgnisBlockoutSettings>()->AssetPrefix;
		TArray<FString> Names;
		for (const TSharedPtr<FJsonObject>& Level : File.Levels)
		{
			const FString Name = GetStringField(*Level, TEXT("name"));
			if ((!Asset->BlockoutLevelName.IsEmpty() && Name == Asset->BlockoutLevelName) || Prefix + SanitizeName(Name) == Asset->GetName())
			{
				return Level;
			}
			Names.Add(Name);
		}

		OutError = FString::Printf(TEXT("El JSON tiene %d niveles y ninguno es de este asset (%s). Exporta solo este nivel (Download this level) o usa \"Importar todos\".\n\nNiveles: %s"),
			File.Levels.Num(), *Asset->GetName(), *FString::Join(Names, TEXT(", ")));
		return nullptr;
	}

	FString ToAbsolutePath(const FString& StoredPath)
	{
		return FPaths::IsRelative(StoredPath) ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), StoredPath) : StoredPath;
	}

	void Notify(const FText& Text, bool bSuccess)
	{
		FNotificationInfo Info(Text);
		Info.ExpireDuration = bSuccess ? 4.f : 8.f;
		Info.bUseSuccessFailIcons = true;
		if (const TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info))
		{
			Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}

	void ShowError(const FString& Error)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Egnis Blockout: ") + Error));
	}

	void ShowWarnings(const FString& Title, const TArray<FString>& Warnings)
	{
		if (Warnings.Num() > 0)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Title + TEXT("\n\nAvisos:\n- ") + FString::Join(Warnings, TEXT("\n- "))));
		}
	}
}

bool ImportIntoAsset(ULevelDataAsset* Asset, const FString& Json, const FString& SourceFile)
{
	if (!Asset)
	{
		return false;
	}

	FParsedFile File;
	FString Error;
	if (!ParseFile(Json, File, Error))
	{
		ShowError(Error);
		return false;
	}

	const TSharedPtr<FJsonObject> Level = PickLevelForAsset(File, Asset, Error);
	if (!Level.IsValid())
	{
		ShowError(Error);
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("ImportLevel", "Importar nivel de Egnis Blockout"));
	TArray<FString> Warnings;
	if (!ApplyLevel(Asset, *Level, File.Size, SourceFile, Warnings, Error))
	{
		ShowError(Error);
		return false;
	}

	const FString Summary = FString::Printf(TEXT("Importado \"%s\": %d x %d, %d enemigos%s. Guarda el asset para conservarlo."),
		*Asset->BlockoutLevelName, Asset->Width, Asset->Height, Asset->EnemiesToSpawn.Num(),
		Asset->SecondaryObjectives.HasAny() ? TEXT(", con objetivos secundarios") : TEXT(""));
	Notify(FText::FromString(Summary), true);
	ShowWarnings(Summary, Warnings);
	return true;
}

bool Reimport(ULevelDataAsset* Asset)
{
	if (!Asset)
	{
		return false;
	}
	if (Asset->BlockoutSourceFile.IsEmpty())
	{
		ShowError(TEXT("Este nivel no se importó desde un archivo: vuelve a pegar el JSON o elige el archivo."));
		return false;
	}

	FString Json;
	const FString FullPath = ToAbsolutePath(Asset->BlockoutSourceFile);
	if (!FFileHelper::LoadFileToString(Json, *FullPath))
	{
		ShowError(FString::Printf(TEXT("No se puede leer %s. Vuelve a exportarlo o elige otro archivo."), *FullPath));
		return false;
	}
	return ImportIntoAsset(Asset, Json, Asset->BlockoutSourceFile);
}

void ImportAll(const FString& Json, const FString& SourceFile)
{
	FParsedFile File;
	FString Error;
	if (!ParseFile(Json, File, Error))
	{
		ShowError(Error);
		return;
	}

	const UEgnisBlockoutSettings* Settings = GetDefault<UEgnisBlockoutSettings>();
	FString Folder = Settings->LevelsFolder.Replace(TEXT("{Zone}"), *FString::FromInt(FMath::Max(File.Zone, 1)));
	Folder.RemoveFromEnd(TEXT("/"));

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> Existing;
	Registry.GetAssetsByPath(FName(*Folder), Existing, true);
	TMap<FString, ULevelDataAsset*> ById;
	for (const FAssetData& Data : Existing)
	{
		if (ULevelDataAsset* LevelAsset = Cast<ULevelDataAsset>(Data.GetAsset()))
		{
			if (!LevelAsset->BlockoutLevelId.IsEmpty())
			{
				ById.Add(LevelAsset->BlockoutLevelId, LevelAsset);
			}
		}
	}

	TArray<UPackage*> PackagesToSave;
	TArray<FString> Warnings;
	TArray<FString> Skipped;
	int32 Created = 0;
	for (const TSharedPtr<FJsonObject>& Level : File.Levels)
	{
		const FString Id = GetStringField(*Level, TEXT("id"));
		const FString Name = GetStringField(*Level, TEXT("name"));
		if (IsBlankLevel(*Level, File.Size))
		{
			Skipped.Add(Name);
			continue;
		}
		ULevelDataAsset* LevelAsset = Id.IsEmpty() ? nullptr : ById.FindRef(Id);

		if (!LevelAsset)
		{
			const FString BaseName = Settings->AssetPrefix + SanitizeName(Name);
			FString AssetName = BaseName;
			for (int32 Suffix = 2; ; ++Suffix)
			{
				const FString PackageName = Folder / AssetName;
				ULevelDataAsset* Found = LoadObject<ULevelDataAsset>(nullptr, *(PackageName + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet);
				if (Found && Found->BlockoutLevelId.IsEmpty())
				{
					LevelAsset = Found;
					break;
				}
				if (!Found && !FPackageName::DoesPackageExist(PackageName))
				{
					UPackage* Package = CreatePackage(*PackageName);
					LevelAsset = NewObject<ULevelDataAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
					FAssetRegistryModule::AssetCreated(LevelAsset);
					++Created;
					break;
				}
				AssetName = FString::Printf(TEXT("%s_%d"), *BaseName, Suffix);
			}
		}
		TArray<FString> LevelWarnings;
		if (!LevelAsset || !ApplyLevel(LevelAsset, *Level, File.Size, SourceFile, LevelWarnings, Error))
		{
			Warnings.Add(FString::Printf(TEXT("%s: %s"), *Name, LevelAsset ? *Error : TEXT("no se ha podido crear el asset")));
			continue;
		}
		for (const FString& Warning : LevelWarnings)
		{
			Warnings.Add(FString::Printf(TEXT("%s: %s"), *Name, *Warning));
		}
		PackagesToSave.AddUnique(LevelAsset->GetOutermost());
	}

	for (UPackage* Package : PackagesToSave)
	{
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		if (!UPackage::SavePackage(Package, nullptr, *Filename, SaveArgs))
		{
			Warnings.Add(FString::Printf(TEXT("No se ha podido guardar %s."), *Package->GetName()));
		}
	}

	FString Summary = FString::Printf(TEXT("Egnis Blockout: %d niveles en %s (%d nuevos, %d actualizados)."),
		PackagesToSave.Num(), *Folder, Created, PackagesToSave.Num() - Created);
	if (Skipped.Num() > 0)
	{
		Summary += FString::Printf(TEXT(" Vacíos, sin importar: %s."), *FString::Join(Skipped, TEXT(", ")));
	}
	Notify(FText::FromString(Summary), PackagesToSave.Num() > 0);
	ShowWarnings(Summary, Warnings);
}

bool ReadClipboard(FString& OutText)
{
	FPlatformApplicationMisc::ClipboardPaste(OutText);
	OutText.TrimStartAndEndInline();
	if (OutText.IsEmpty())
	{
		ShowError(TEXT("El portapapeles está vacío. En Egnis Blockout: Export -> Ver JSON de todos -> Copy text."));
		return false;
	}
	return true;
}

bool PickJsonFile(FString& OutStoredPath, FString& OutText)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return false;
	}

	TArray<FString> Files;
	const void* ParentWindow = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	if (!DesktopPlatform->OpenFileDialog(ParentWindow, TEXT("JSON de Egnis Blockout"), FPaths::ProjectDir(), TEXT(""),
		TEXT("JSON (*.json)|*.json"), EFileDialogFlags::None, Files) || Files.Num() == 0)
	{
		return false;
	}

	const FString FullPath = FPaths::ConvertRelativePathToFull(Files[0]);
	if (!FFileHelper::LoadFileToString(OutText, *FullPath))
	{
		ShowError(FString::Printf(TEXT("No se puede leer %s."), *FullPath));
		return false;
	}

	// Dentro del proyecto se guarda la ruta relativa, así reimportar funciona en el ordenador de cualquiera
	OutStoredPath = FullPath;
	const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	if (OutStoredPath.StartsWith(ProjectDir))
	{
		FPaths::MakePathRelativeTo(OutStoredPath, *ProjectDir);
	}
	return true;
}

void ImportAllFromFile()
{
	FString StoredPath;
	FString Json;
	if (PickJsonFile(StoredPath, Json))
	{
		ImportAll(Json, StoredPath);
	}
}
}

#undef LOCTEXT_NAMESPACE
