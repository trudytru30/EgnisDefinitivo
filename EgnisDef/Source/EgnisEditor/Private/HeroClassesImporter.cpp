#include "HeroClassesImporter.h"
#include "EgnisEditor.h"
#include "EgnisSheetsSettings.h"
#include "Characters/HeroClassData.h"
#include "Engine/DataTable.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/Csv/CsvParser.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace
{
	void Notify(const FString& Message, bool bSuccess)
	{
		FNotificationInfo Info(FText::FromString(Message));
		Info.ExpireDuration = 6.f;
		Info.bUseSuccessFailIcons = true;
		if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info))
		{
			Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}

	bool ParseNumber(const FString& Cell, float& OutValue)
	{
		const FString Clean = Cell.TrimStartAndEnd().Replace(TEXT(","), TEXT("."));
		if (Clean.IsEmpty() || !FCString::IsNumeric(*Clean))
		{
			return false;
		}
		OutValue = FCString::Atof(*Clean);
		return true;
	}

	FString GetCell(const TArray<const TCHAR*>& Row, int32 Column)
	{
		return Row.IsValidIndex(Column) ? FString(Row[Column]).TrimStartAndEnd() : FString();
	}
}

void UHeroClassesImporter::SyncFromGoogleSheets()
{
	const UEgnisSheetsSettings* Settings = GetDefault<UEgnisSheetsSettings>();
	UDataTable* Table = Settings->HeroClassesTable.LoadSynchronous();
	if (Settings->HeroClassesCSVUrl.IsEmpty() || !Table)
	{
		UE_LOG(HeadCrackEditor, Error, TEXT("[ClassImporter]: Falta la URL o la tabla de clases en Project Settings > Egnis > Sheets Importer."));
		Notify(TEXT("Clases: falta la URL o la tabla en Project Settings"), false);
		return;
	}

	TWeakObjectPtr<UDataTable> WeakTable(Table);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindLambda([WeakTable](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bSuccess)
	{
		if (!bSuccess || !Response.IsValid() || Response->GetResponseCode() != 200)
		{
			UE_LOG(HeadCrackEditor, Error, TEXT("[ClassImporter]: No se pudo descargar el CSV de clases."));
			Notify(TEXT("Clases: no se pudo descargar el CSV"), false);
			return;
		}

		UDataTable* LoadedTable = WeakTable.Get();
		if (!LoadedTable)
		{
			return;
		}

		const int32 Updated = ImportFromCSV(Response->GetContentAsString(), LoadedTable);
		if (Updated > 0)
		{
			Notify(FString::Printf(TEXT("Clases: %d filas actualizadas en %s (falta guardar)"), Updated, *LoadedTable->GetName()), true);
		}
		else
		{
			Notify(TEXT("Clases: no se ha actualizado ninguna fila, mira el Output Log"), false);
		}
	});

	Request->SetURL(Settings->HeroClassesCSVUrl);
	Request->SetVerb(TEXT("GET"));
	Request->ProcessRequest();
}

int32 UHeroClassesImporter::ImportFromCSV(const FString& CSV, UDataTable* Table)
{
	if (!Table || Table->GetRowStruct() != FHeroClassData::StaticStruct())
	{
		UE_LOG(HeadCrackEditor, Error, TEXT("[ClassImporter]: La tabla no es de tipo FHeroClassData."));
		return -1;
	}

	const UEgnisSheetsSettings* Settings = GetDefault<UEgnisSheetsSettings>();
	const FCsvParser Parser(CSV);
	const FCsvParser::FRows& Rows = Parser.GetRows();

	int32 HeaderRow = INDEX_NONE;
	int32 ClassColumn = INDEX_NONE;
	bool bInBlock = false;
	for (int32 RowIndex = 0; RowIndex < Rows.Num() && HeaderRow == INDEX_NONE; ++RowIndex)
	{
		for (int32 Column = 0; Column < Rows[RowIndex].Num(); ++Column)
		{
			const FString Cell = GetCell(Rows[RowIndex], Column);
			if (!bInBlock && Cell.Equals(Settings->ClassStatsBlockTitle, ESearchCase::IgnoreCase))
			{
				bInBlock = true;
				break;
			}
			if (bInBlock && Cell.Equals(TEXT("CLASS"), ESearchCase::IgnoreCase))
			{
				HeaderRow = RowIndex;
				ClassColumn = Column;
				break;
			}
		}
	}

	if (HeaderRow == INDEX_NONE)
	{
		UE_LOG(HeadCrackEditor, Error, TEXT("[ClassImporter]: No se encuentra el bloque '%s' con su fila de cabecera CLASS."), *Settings->ClassStatsBlockTitle);
		return -1;
	}

	const TArray<FString> StatHeaders = { TEXT("LIFE"), TEXT("POWER"), TEXT("PIETY"), TEXT("SPEED"), TEXT("RANGE") };
	TArray<int32> StatColumns;
	for (const FString& Header : StatHeaders)
	{
		int32 Found = INDEX_NONE;
		for (int32 Column = 0; Column < Rows[HeaderRow].Num(); ++Column)
		{
			if (GetCell(Rows[HeaderRow], Column).Equals(Header, ESearchCase::IgnoreCase))
			{
				Found = Column;
				break;
			}
		}
		if (Found == INDEX_NONE)
		{
			UE_LOG(HeadCrackEditor, Error, TEXT("[ClassImporter]: Falta la columna %s en el bloque '%s'."), *Header, *Settings->ClassStatsBlockTitle);
			return -1;
		}
		StatColumns.Add(Found);
	}

	Table->Modify();

	int32 Updated = 0;
	for (int32 RowIndex = HeaderRow + 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const FString ClassName = GetCell(Rows[RowIndex], ClassColumn);
		if (ClassName.IsEmpty())
		{
			break;
		}

		const FName* RowName = Settings->HeroClassRowNames.Find(ClassName);
		if (!RowName)
		{
			UE_LOG(HeadCrackEditor, Warning, TEXT("[ClassImporter]: %s no tiene fila asignada en Project Settings, se salta."), *ClassName);
			continue;
		}

		FHeroClassData* Data = Table->FindRow<FHeroClassData>(*RowName, TEXT("ClassImporter"), false);
		if (!Data)
		{
			UE_LOG(HeadCrackEditor, Warning, TEXT("[ClassImporter]: %s -> %s no existe en %s, se salta."), *ClassName, *RowName->ToString(), *Table->GetName());
			continue;
		}

		float Values[5];
		bool bValid = true;
		for (int32 Stat = 0; Stat < StatColumns.Num(); ++Stat)
		{
			if (!ParseNumber(GetCell(Rows[RowIndex], StatColumns[Stat]), Values[Stat]))
			{
				UE_LOG(HeadCrackEditor, Warning, TEXT("[ClassImporter]: %s tiene %s vacío o no numérico, se salta."), *ClassName, *StatHeaders[Stat]);
				bValid = false;
				break;
			}
		}
		if (!bValid)
		{
			continue;
		}

		Data->Vitality = Values[0];
		Data->Power = Values[1];
		Data->Piety = Values[2];
		Data->Speed = FMath::RoundToInt(Values[3]);
		Data->Range = FMath::RoundToInt(Values[4]);
		++Updated;

		UE_LOG(HeadCrackEditor, Log, TEXT("[ClassImporter]: %s -> %s: vida %.2f, poder %.2f, piedad %.2f, velocidad %d, rango %d"),
			*ClassName, *RowName->ToString(), Data->Vitality, Data->Power, Data->Piety, Data->Speed, Data->Range);
	}

	if (Updated > 0)
	{
		Table->HandleDataTableChanged();
		Table->MarkPackageDirty();
	}

	UE_LOG(HeadCrackEditor, Log, TEXT("[ClassImporter]: %d filas actualizadas en %s."), Updated, *Table->GetName());
	return Updated;
}
