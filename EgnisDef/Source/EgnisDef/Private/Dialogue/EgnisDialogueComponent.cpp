#include "Dialogue/EgnisDialogueComponent.h"

#include "Dialogue/EgnisDialogueWidget.h"
#include "CoreAndSystems/GameTextRow.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisDialogue, Log, All);

namespace EgnisDialogue
{
	static const TCHAR* SheetBaseUrl =
		TEXT("https://docs.google.com/spreadsheets/d/1UkfEbO6lb73EIsDXsCsDZ2hyWf9BoLEvKA40BRV2v-U/export?format=csv&gid=");

	// Pestañas del Excel: TheGap_Default, Main_Missions, Events, IntrosOutros
	static const TCHAR* SheetGids[] = { TEXT("1789220314"), TEXT("943484554"), TEXT("925726913"), TEXT("1375938643") };

	static FString CleanCell(const FString& In)
	{
		FString Out = In.TrimStartAndEnd();
		if (Out.StartsWith(TEXT("#VALUE!")) || Out.StartsWith(TEXT("#REF!")) || Out.StartsWith(TEXT("#N/A")))
		{
			return FString();
		}
		return Out;
	}

	static void ScreenMessage(const FString& Message, const FColor& Color, float Time = 5.f)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Time, Color, Message);
		}
	}
}

UEgnisDialogueComponent::UEgnisDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	for (const TCHAR* Gid : EgnisDialogue::SheetGids)
	{
		GoogleSheetCsvUrls.Add(FString(EgnisDialogue::SheetBaseUrl) + Gid);
	}
}

// --------------------------------------------------------------------------------------
// Ciclo de vida
// --------------------------------------------------------------------------------------

void UEgnisDialogueComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		if (bStartOnPlayerOverlap)
		{
			CreateTriggerIfNeeded();
			Owner->OnActorBeginOverlap.AddDynamic(this, &UEgnisDialogueComponent::HandleOwnerBeginOverlap);
		}
	}

	ReloadData();

	if (bStartOnBeginPlay)
	{
		StartDialogue(StartRowID);
	}
}

void UEgnisDialogueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	++LoadGeneration; // invalida descargas pendientes
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	bActive = false;
	Super::EndPlay(EndPlayReason);
}

void UEgnisDialogueComponent::CreateTriggerIfNeeded()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (Prim && Prim->GetGenerateOverlapEvents() && Prim->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			return; // ya tiene algo con lo que solapar
		}
	}

	USphereComponent* Sphere = NewObject<USphereComponent>(Owner, TEXT("EgnisDialogueTrigger"));
	Sphere->SetSphereRadius(TriggerRadius);
	Sphere->SetCollisionProfileName(TEXT("Trigger"));
	Sphere->SetGenerateOverlapEvents(true);
	if (USceneComponent* Root = Owner->GetRootComponent())
	{
		Sphere->SetupAttachment(Root);
	}
	Sphere->RegisterComponent();
	Owner->AddInstanceComponent(Sphere);
	if (!Owner->GetRootComponent())
	{
		Owner->SetRootComponent(Sphere);
	}
}

void UEgnisDialogueComponent::HandleOwnerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled() || bActive)
	{
		return;
	}
	if (bOnlyOnce && bAlreadyPlayed)
	{
		return;
	}
	StartDialogue(StartRowID);
}

// --------------------------------------------------------------------------------------
// Carga de datos
// --------------------------------------------------------------------------------------

void UEgnisDialogueComponent::ReloadData()
{
	++LoadGeneration;
	Rows.Reset();
	RowIndexByID.Reset();
	bDataLoaded = false;

	LoadFromDataTables();

	if (!bDownloadFromGoogleSheets || GoogleSheetCsvUrls.Num() == 0)
	{
		OnAllSourcesLoaded();
		return;
	}

	PendingDownloads = GoogleSheetCsvUrls.Num();
	const int32 Generation = LoadGeneration;
	TWeakObjectPtr<UEgnisDialogueComponent> WeakThis(this);

	for (int32 Index = 0; Index < GoogleSheetCsvUrls.Num(); ++Index)
	{
		const FString Url = GoogleSheetCsvUrls[Index];
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(Url);
		Request->SetVerb(TEXT("GET"));
		Request->SetTimeout(15.f);
		Request->OnProcessRequestComplete().BindLambda(
			[WeakThis, Generation, Index, Url](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
			{
				UEgnisDialogueComponent* Self = WeakThis.Get();
				if (!Self || Self->LoadGeneration != Generation)
				{
					return;
				}

				const FString CachePath = Self->GetCachePath(Index);
				FString Csv;
				const bool bGood = bSuccess && Resp.IsValid() && Resp->GetResponseCode() == 200;
				if (bGood)
				{
					Csv = Resp->GetContentAsString();
				}

				if (bGood && !Csv.TrimStart().StartsWith(TEXT("<")))
				{
					FFileHelper::SaveStringToFile(Csv, *CachePath, FFileHelper::EEncodingOptions::ForceUTF8);
					Self->LoadFromCsv(Csv, FString::Printf(TEXT("Sheet%d"), Index));
				}
				else if (FFileHelper::LoadFileToString(Csv, *CachePath))
				{
					UE_LOG(LogEgnisDialogue, Warning, TEXT("No se pudo descargar %s. Uso la copia local %s"), *Url, *CachePath);
					Self->LoadFromCsv(Csv, FString::Printf(TEXT("Sheet%d (cache)"), Index));
				}
				else
				{
					UE_LOG(LogEgnisDialogue, Error, TEXT("No se pudo descargar %s y no hay copia local."), *Url);
					EgnisDialogue::ScreenMessage(TEXT("[Dialogos] No se pudo descargar el Excel (¿sin conexion o sin permisos?)"), FColor::Red, 8.f);
				}

				if (--Self->PendingDownloads <= 0)
				{
					Self->OnAllSourcesLoaded();
				}
			});
		Request->ProcessRequest();
	}
}

FString UEgnisDialogueComponent::GetCachePath(int32 Index) const
{
	return FPaths::ProjectSavedDir() / TEXT("DialogueCache") / FString::Printf(TEXT("sheet_%d.csv"), Index);
}

void UEgnisDialogueComponent::LoadFromDataTables()
{
	for (UDataTable* Table : DialogueTables)
	{
		if (!Table || Table->GetRowStruct() != FGameTextRow::StaticStruct())
		{
			if (Table)
			{
				UE_LOG(LogEgnisDialogue, Warning, TEXT("%s no usa la estructura GameTextRow, se ignora."), *Table->GetName());
			}
			continue;
		}

		for (const FName& Name : Table->GetRowNames())
		{
			const FGameTextRow* Src = Table->FindRow<FGameTextRow>(Name, TEXT("EgnisDialogue"));
			if (!Src)
			{
				continue;
			}
			FEgnisDialogueRow Row;
			Row.RowID = Name;
			Row.NextRowID = Src->Next_Row_ID;
			Row.CharacterID = Src->Character_ID.IsNone() ? FString() : Src->Character_ID.ToString();
			Row.Type = Src->Type;
			Row.ParentRowID = Src->Parent_Row_ID;
			Row.Chance = Src->Chance;
			Row.Conditions = Src->Conditions;
			Row.AudioCue = Src->Audio_Cue;
			Row.Actions = Src->Actions;
			Row.TextEN = EgnisDialogue::CleanCell(Src->Text_en);
			Row.TextES = EgnisDialogue::CleanCell(Src->Text_es);
			Row.SourceSheet = Table->GetName();
			AddRow(Row);
		}
	}
}

void UEgnisDialogueComponent::LoadFromCsv(const FString& Csv, const FString& SheetName)
{
	TArray<TArray<FString>> Table;
	ParseCsv(Csv, Table);

	// Columnas por defecto (por si la hoja no tiene cabecera)
	TMap<FString, int32> Col;
	const TCHAR* Names[] = { TEXT("Row_ID"), TEXT("Next_Row_ID"), TEXT("Character_ID"), TEXT("Type"), TEXT("Parent_Row_ID"),
		TEXT("Chance"), TEXT("Conditions"), TEXT("Audio_Cue"), TEXT("Actions"), TEXT("Text_en"), TEXT("Text_es") };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Names); ++i)
	{
		Col.Add(Names[i], i);
	}

	int32 FirstDataRow = 0;
	for (int32 r = 0; r < Table.Num(); ++r)
	{
		if (Table[r].Num() > 0 && Table[r][0].TrimStartAndEnd().Equals(TEXT("Row_ID"), ESearchCase::IgnoreCase))
		{
			// Algunas pestañas tienen la cabecera repetida más a la derecha: vale la primera.
			TSet<FString> Seen;
			for (int32 c = 0; c < Table[r].Num(); ++c)
			{
				const FString Header = Table[r][c].TrimStartAndEnd();
				if (!Header.IsEmpty() && !Seen.Contains(Header))
				{
					Seen.Add(Header);
					Col.Add(Header, c);
				}
			}
			FirstDataRow = r + 1;
			break;
		}
	}

	auto Cell = [&Col](const TArray<FString>& Line, const TCHAR* Name) -> FString
	{
		const int32* Idx = Col.Find(Name);
		return (Idx && Line.IsValidIndex(*Idx)) ? EgnisDialogue::CleanCell(Line[*Idx]) : FString();
	};

	int32 Added = 0;
	for (int32 r = FirstDataRow; r < Table.Num(); ++r)
	{
		const TArray<FString>& Line = Table[r];
		const FString Id = Cell(Line, TEXT("Row_ID"));
		if (Id.IsEmpty() || Id.Equals(TEXT("Row_ID"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		FEgnisDialogueRow Row;
		Row.RowID = FName(*Id);
		const FString Next = Cell(Line, TEXT("Next_Row_ID"));
		Row.NextRowID = Next.IsEmpty() ? NAME_None : FName(*Next);
		Row.CharacterID = Cell(Line, TEXT("Character_ID"));
		Row.Type = Cell(Line, TEXT("Type"));
		const FString Parent = Cell(Line, TEXT("Parent_Row_ID"));
		Row.ParentRowID = Parent.IsEmpty() ? NAME_None : FName(*Parent);
		const FString Chance = Cell(Line, TEXT("Chance"));
		Row.Chance = Chance.IsEmpty() ? 0.f : FCString::Atof(*Chance.Replace(TEXT(","), TEXT(".")));
		Row.Conditions = Cell(Line, TEXT("Conditions"));
		Row.AudioCue = Cell(Line, TEXT("Audio_Cue"));
		Row.Actions = Cell(Line, TEXT("Actions"));
		Row.TextEN = Cell(Line, TEXT("Text_en"));
		Row.TextES = Cell(Line, TEXT("Text_es"));
		Row.SourceSheet = SheetName;
		if (Row.Type.IsEmpty())
		{
			Row.Type = TEXT("Line");
		}
		AddRow(Row);
		++Added;
	}

	UE_LOG(LogEgnisDialogue, Log, TEXT("%s: %d filas de dialogo cargadas."), *SheetName, Added);
}

void UEgnisDialogueComponent::AddRow(const FEgnisDialogueRow& Row)
{
	if (RowIndexByID.Contains(Row.RowID))
	{
		UE_LOG(LogEgnisDialogue, Warning, TEXT("Row_ID repetido '%s' (%s). Me quedo con el primero."), *Row.RowID.ToString(), *Row.SourceSheet);
		return;
	}
	const int32 Index = Rows.Add(Row);
	RowIndexByID.Add(Row.RowID, Index);
}

void UEgnisDialogueComponent::OnAllSourcesLoaded()
{
	bDataLoaded = true;
	UE_LOG(LogEgnisDialogue, Log, TEXT("Dialogos listos: %d filas."), Rows.Num());

	if (bHasPendingStart)
	{
		bHasPendingStart = false;
		StartDialogue(PendingStartRow);
	}
}

void UEgnisDialogueComponent::ParseCsv(const FString& Csv, TArray<TArray<FString>>& OutRows)
{
	OutRows.Reset();
	TArray<FString> Current;
	FString Field;
	bool bInQuotes = false;

	const int32 Len = Csv.Len();
	for (int32 i = 0; i < Len; ++i)
	{
		const TCHAR C = Csv[i];
		if (bInQuotes)
		{
			if (C == TEXT('"'))
			{
				if (i + 1 < Len && Csv[i + 1] == TEXT('"'))
				{
					Field.AppendChar(TEXT('"'));
					++i;
				}
				else
				{
					bInQuotes = false;
				}
			}
			else
			{
				Field.AppendChar(C);
			}
			continue;
		}

		if (C == TEXT('"'))
		{
			bInQuotes = true;
		}
		else if (C == TEXT(','))
		{
			Current.Add(Field);
			Field.Reset();
		}
		else if (C == TEXT('\r'))
		{
			// se ignora, el \n cierra la fila
		}
		else if (C == TEXT('\n'))
		{
			Current.Add(Field);
			Field.Reset();
			OutRows.Add(MoveTemp(Current));
			Current.Reset();
		}
		else if (C == 0xFEFF)
		{
			// BOM
		}
		else
		{
			Field.AppendChar(C);
		}
	}

	if (!Field.IsEmpty() || Current.Num() > 0)
	{
		Current.Add(Field);
		OutRows.Add(MoveTemp(Current));
	}
}

// --------------------------------------------------------------------------------------
// Consultas
// --------------------------------------------------------------------------------------

int32 UEgnisDialogueComponent::FindRowIndex(FName RowID) const
{
	if (const int32* Found = RowIndexByID.Find(RowID))
	{
		return *Found;
	}
	return INDEX_NONE;
}

bool UEgnisDialogueComponent::FindRow(FName RowID, FEgnisDialogueRow& OutRow) const
{
	const int32 Index = FindRowIndex(RowID);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutRow = Rows[Index];
	return true;
}

bool UEgnisDialogueComponent::IsEndToken(FName RowID)
{
	return RowID.IsNone() || RowID.ToString().Equals(TEXT("END"), ESearchCase::IgnoreCase);
}

bool UEgnisDialogueComponent::EvaluateConditions(const FString& Conditions) const
{
	TArray<FString> Parts;
	Conditions.ParseIntoArray(Parts, TEXT("&"), true);
	for (FString Part : Parts)
	{
		Part.TrimStartAndEndInline();
		if (Part.IsEmpty())
		{
			continue;
		}
		bool bNegate = false;
		while (Part.StartsWith(TEXT("!")))
		{
			bNegate = !bNegate;
			Part.RightChopInline(1);
			Part.TrimStartInline();
		}
		const bool* Value = Flags.Find(FName(*Part));
		const bool bFlag = Value ? *Value : false;
		if (bFlag == bNegate)
		{
			return false;
		}
	}
	return true;
}

TArray<FName> UEgnisDialogueComponent::GetDialogueEntryPoints() const
{
	TSet<FName> Referenced;
	for (const FEgnisDialogueRow& Row : Rows)
	{
		Referenced.Add(Row.NextRowID);
		Referenced.Add(Row.ParentRowID);
	}

	TArray<FName> Result;
	for (const FEgnisDialogueRow& Row : Rows)
	{
		if (Row.IsType(TEXT("Choice")) || Row.IsType(TEXT("Variant")))
		{
			continue;
		}
		if (!Referenced.Contains(Row.RowID))
		{
			Result.Add(Row.RowID);
		}
	}
	return Result;
}

FText UEgnisDialogueComponent::GetRowText(const FEgnisDialogueRow& Row) const
{
	FString Text = bUseSpanish ? Row.TextES : Row.TextEN;
	if (Text.IsEmpty())
	{
		Text = bUseSpanish ? Row.TextEN : Row.TextES;
	}
	if (Text.IsEmpty())
	{
		Text = FString::Printf(TEXT("[%s: fila sin texto]"), *Row.RowID.ToString());
	}
	return FText::FromString(Text);
}

FText UEgnisDialogueComponent::GetSpeakerText(const FEgnisDialogueRow& Row) const
{
	if (Row.CharacterID.IsEmpty() || Row.CharacterID.Equals(TEXT("Player"), ESearchCase::IgnoreCase))
	{
		return FText::GetEmpty();
	}
	if (bUseSpanish && Row.CharacterID.Equals(TEXT("Storyteller"), ESearchCase::IgnoreCase))
	{
		return FText::FromString(TEXT("Narrador"));
	}
	return FText::FromString(Row.CharacterID);
}

// --------------------------------------------------------------------------------------
// Flujo del diálogo
// --------------------------------------------------------------------------------------

void UEgnisDialogueComponent::StartDialogue(FName RowID)
{
	if (RowID.IsNone())
	{
		RowID = StartRowID;
	}

	if (!bDataLoaded)
	{
		// Todavía se está descargando el Excel: arranca en cuanto termine.
		PendingStartRow = RowID;
		bHasPendingStart = true;
		EgnisDialogue::ScreenMessage(TEXT("[Dialogos] Cargando el Excel..."), FColor::Yellow, 2.f);
		return;
	}

	if (FindRowIndex(RowID) == INDEX_NONE && FindRowIndex(FName(*(RowID.ToString() + TEXT("_01")))) == INDEX_NONE)
	{
		const FString Msg = FString::Printf(TEXT("[Dialogos] No existe la fila '%s' en el Excel."), *RowID.ToString());
		UE_LOG(LogEgnisDialogue, Error, TEXT("%s"), *Msg);
		EgnisDialogue::ScreenMessage(Msg, FColor::Red, 8.f);
		return;
	}

	const bool bWasActive = bActive;
	bActive = true;
	bAlreadyPlayed = true;
	EnsureWidget();
	if (!bWasActive)
	{
		SetUIInput(true);
	}
	OnDialogueStarted.Broadcast(RowID);
	GoTo(RowID);
}

void UEgnisDialogueComponent::GoTo(FName RowID, int32 Depth)
{
	if (!bActive)
	{
		return;
	}
	if (Depth > 32)
	{
		UE_LOG(LogEgnisDialogue, Error, TEXT("Bucle en el dialogo cerca de '%s'."), *RowID.ToString());
		EndDialogue();
		return;
	}
	if (IsEndToken(RowID))
	{
		EndDialogue();
		return;
	}

	int32 Index = FindRowIndex(RowID);
	if (Index == INDEX_NONE)
	{
		// Algunas filas apuntan a "X" cuando la fila real se llama "X_01"
		Index = FindRowIndex(FName(*(RowID.ToString() + TEXT("_01"))));
	}
	if (Index == INDEX_NONE)
	{
		const FString Msg = FString::Printf(TEXT("[Dialogos] La fila '%s' no existe en el Excel."), *RowID.ToString());
		UE_LOG(LogEgnisDialogue, Error, TEXT("%s"), *Msg);
		EgnisDialogue::ScreenMessage(Msg, FColor::Red, 8.f);
		EndDialogue();
		return;
	}

	const FEgnisDialogueRow& Row = Rows[Index];

	// Condición falsa
	if (!EvaluateConditions(Row.Conditions))
	{
		if (!Row.ParentRowID.IsNone() && !Row.IsType(TEXT("Variant")) && !Row.IsType(TEXT("Choice")))
		{
			GoTo(Row.ParentRowID, Depth + 1);
			return;
		}
		// Busca una alternativa: otra fila del mismo bloque que lleve al mismo sitio
		if (!Row.NextRowID.IsNone())
		{
			for (int32 i = Index + 1; i < Rows.Num() && Rows[i].SourceSheet == Row.SourceSheet; ++i)
			{
				if (Rows[i].NextRowID != Row.NextRowID)
				{
					break;
				}
				if (EvaluateConditions(Rows[i].Conditions))
				{
					GoTo(Rows[i].RowID, Depth + 1);
					return;
				}
			}
		}
		GoTo(Row.NextRowID, Depth + 1);
		return;
	}

	if (Row.IsType(TEXT("Random")))
	{
		const int32 Variant = PickVariant(Row);
		if (Variant != INDEX_NONE)
		{
			ShowLine(Rows[Variant]);
		}
		else
		{
			GoTo(Row.NextRowID, Depth + 1);
		}
		return;
	}

	if (Row.IsType(TEXT("Variant")) && !Row.ParentRowID.IsNone())
	{
		// Si se llega directamente a una Variant, se sortea entre todas las de su Random
		const int32 ParentIndex = FindRowIndex(Row.ParentRowID);
		if (ParentIndex != INDEX_NONE && Rows[ParentIndex].IsType(TEXT("Random")))
		{
			GoTo(Row.ParentRowID, Depth + 1);
			return;
		}
	}

	if (Row.IsType(TEXT("Choice")))
	{
		// Se ha llegado a una opción desde Next_Row_ID: muestra la línea que la plantea
		if (!Row.ParentRowID.IsNone() && FindRowIndex(Row.ParentRowID) != INDEX_NONE)
		{
			GoTo(Row.ParentRowID, Depth + 1);
		}
		else
		{
			GoTo(Row.NextRowID, Depth + 1);
		}
		return;
	}

	ShowLine(Row);
}

int32 UEgnisDialogueComponent::PickVariant(const FEgnisDialogueRow& RandomRow) const
{
	FString Alias = RandomRow.RowID.ToString();
	Alias.RemoveFromEnd(TEXT("_01"));
	const FName AliasName(*Alias);

	TArray<int32> Candidates;
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		const FEgnisDialogueRow& Row = Rows[i];
		if (Row.IsType(TEXT("Variant")) && (Row.ParentRowID == RandomRow.RowID || Row.ParentRowID == AliasName))
		{
			Candidates.Add(i);
		}
	}

	// Si las Variant no tienen Parent_Row_ID, se usan las que vienen detrás del Random
	if (Candidates.Num() == 0)
	{
		const int32 Start = FindRowIndex(RandomRow.RowID);
		for (int32 i = Start + 1; i < Rows.Num() && Rows[i].SourceSheet == RandomRow.SourceSheet; ++i)
		{
			if (Rows[i].IsType(TEXT("Random")))
			{
				break;
			}
			if (Rows[i].IsType(TEXT("Variant")) && Rows[i].ParentRowID.IsNone())
			{
				Candidates.Add(i);
			}
		}
	}

	float Total = 0.f;
	TArray<TPair<int32, float>> Weighted;
	for (int32 i : Candidates)
	{
		if (!EvaluateConditions(Rows[i].Conditions))
		{
			continue;
		}
		const float W = Rows[i].Chance > 0.f ? Rows[i].Chance : 1.f;
		Weighted.Emplace(i, W);
		Total += W;
	}

	if (Weighted.Num() == 0)
	{
		return INDEX_NONE;
	}

	float Roll = FMath::FRand() * Total;
	for (const TPair<int32, float>& Pair : Weighted)
	{
		Roll -= Pair.Value;
		if (Roll <= 0.f)
		{
			return Pair.Key;
		}
	}
	return Weighted.Last().Key;
}

void UEgnisDialogueComponent::GatherChoices(const FEgnisDialogueRow& Row, TArray<int32>& OutChoiceIndices) const
{
	OutChoiceIndices.Reset();
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		if (Rows[i].IsType(TEXT("Choice")) && Rows[i].ParentRowID == Row.RowID && EvaluateConditions(Rows[i].Conditions))
		{
			OutChoiceIndices.Add(i);
		}
	}

	// Líneas que terminan en opciones pero cuyas opciones no tienen bien el Parent_Row_ID:
	// se usan las Choice que van justo detrás en el Excel.
	if (OutChoiceIndices.Num() == 0 && Row.NextRowID.IsNone())
	{
		const int32 Start = FindRowIndex(Row.RowID);
		for (int32 i = Start + 1; i < Rows.Num() && Rows[i].SourceSheet == Row.SourceSheet && Rows[i].IsType(TEXT("Choice")); ++i)
		{
			if (EvaluateConditions(Rows[i].Conditions))
			{
				OutChoiceIndices.Add(i);
			}
		}
	}
}

void UEgnisDialogueComponent::ShowLine(const FEgnisDialogueRow& Row)
{
	FireRowEvents(Row);

	TArray<int32> ChoiceIndices;
	GatherChoices(Row, ChoiceIndices);

	CurrentLine = FEgnisDialogueLine();
	CurrentLine.RowID = Row.RowID;
	CurrentLine.Speaker = GetSpeakerText(Row);
	CurrentLine.Text = GetRowText(Row);
	CurrentChoiceTargets.Reset();
	for (int32 i : ChoiceIndices)
	{
		FEgnisDialogueChoice Choice;
		Choice.RowID = Rows[i].RowID;
		Choice.Text = GetRowText(Rows[i]);
		CurrentLine.Choices.Add(Choice);
		CurrentChoiceTargets.Add(Rows[i].RowID);
	}
	CurrentNextRowID = Row.NextRowID;
	CurrentLine.bIsLast = ChoiceIndices.Num() == 0 && IsEndToken(Row.NextRowID);

	UE_LOG(LogEgnisDialogue, Verbose, TEXT("[%s] %s"), *Row.RowID.ToString(), *CurrentLine.Text.ToString());

	EnsureWidget();
	if (Widget)
	{
		Widget->ShowLine(CurrentLine);
	}
	OnLineShown.Broadcast(CurrentLine);
}

void UEgnisDialogueComponent::FireRowEvents(const FEgnisDialogueRow& Row)
{
	if (!Row.Actions.IsEmpty())
	{
		// Acciones del tipo "Flag = true" / "Flag = false" (separadas por ';' o saltos de línea)
		// cambian directamente los flags que usa la columna Conditions.
		TArray<FString> Parts;
		Row.Actions.Replace(TEXT("\n"), TEXT(";")).ParseIntoArray(Parts, TEXT(";"), true);
		for (const FString& Part : Parts)
		{
			FString Left, Right;
			if (!Part.Split(TEXT("="), &Left, &Right))
			{
				continue;
			}
			Left.TrimStartAndEndInline();
			Right.TrimStartAndEndInline();
			if (Left.IsEmpty() || Left.Contains(TEXT(" ")))
			{
				continue;
			}
			bool bValue;
			if (Right.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Right == TEXT("1"))
			{
				bValue = true;
			}
			else if (Right.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Right == TEXT("0"))
			{
				bValue = false;
			}
			else
			{
				continue;
			}
			SetFlag(FName(*Left), bValue);
			UE_LOG(LogEgnisDialogue, Log, TEXT("Flag %s = %s (fila %s)"), *Left, bValue ? TEXT("true") : TEXT("false"), *Row.RowID.ToString());
		}

		OnDialogueAction.Broadcast(Row.Actions, Row.RowID);
		if (bPrintActionsOnScreen)
		{
			EgnisDialogue::ScreenMessage(FString::Printf(TEXT("[Dialogos] Accion: %s"), *Row.Actions), FColor::Cyan);
		}
	}
	if (!Row.AudioCue.IsEmpty())
	{
		OnAudioCue.Broadcast(Row.AudioCue, Row.RowID);
		if (bPrintActionsOnScreen)
		{
			EgnisDialogue::ScreenMessage(FString::Printf(TEXT("[Dialogos] Audio: %s"), *Row.AudioCue), FColor::Cyan);
		}
	}
}

void UEgnisDialogueComponent::Advance()
{
	if (!bActive || CurrentLine.Choices.Num() > 0)
	{
		return;
	}
	GoTo(CurrentNextRowID);
}

void UEgnisDialogueComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!bActive || !CurrentChoiceTargets.IsValidIndex(ChoiceIndex))
	{
		return;
	}
	const int32 Index = FindRowIndex(CurrentChoiceTargets[ChoiceIndex]);
	if (Index == INDEX_NONE)
	{
		EndDialogue();
		return;
	}
	const FEgnisDialogueRow ChoiceRow = Rows[Index];
	FireRowEvents(ChoiceRow);
	GoTo(ChoiceRow.NextRowID);
}

void UEgnisDialogueComponent::EndDialogue()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	CurrentLine = FEgnisDialogueLine();
	CurrentChoiceTargets.Reset();
	if (Widget)
	{
		Widget->RemoveFromParent();
	}
	SetUIInput(false);
	OnDialogueEnded.Broadcast();
}

void UEgnisDialogueComponent::SetFlag(FName Flag, bool bValue)
{
	Flags.Add(Flag, bValue);
}

// --------------------------------------------------------------------------------------
// UI
// --------------------------------------------------------------------------------------

void UEgnisDialogueComponent::EnsureWidget()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	if (!Widget)
	{
		UClass* Class = WidgetClass ? WidgetClass.Get() : UEgnisDialogueWidget::StaticClass();
		Widget = CreateWidget<UEgnisDialogueWidget>(PC, Class);
		if (!Widget)
		{
			return;
		}
		Widget->SetDialogueComponent(this);
	}

	if (!Widget->IsInViewport())
	{
		Widget->AddToViewport(50);
	}
}

void UEgnisDialogueComponent::SetUIInput(bool bEnable)
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	if (bEnable)
	{
		bPreviousShowCursor = PC->bShowMouseCursor;
		FInputModeGameAndUI Mode;
		if (Widget)
		{
			Mode.SetWidgetToFocus(Widget->TakeWidget());
		}
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		if (bPreviousShowCursor)
		{
			FInputModeGameAndUI Mode;
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
		}
		else
		{
			PC->SetInputMode(FInputModeGameOnly());
		}
		PC->SetShowMouseCursor(bPreviousShowCursor);
	}
}

// --------------------------------------------------------------------------------------
// Comandos de consola para probar
// --------------------------------------------------------------------------------------

namespace EgnisDialogue
{
	static UEgnisDialogueComponent* FindOrCreate(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TObjectIterator<UEgnisDialogueComponent> It; It; ++It)
		{
			if (It->GetWorld() == World && It->IsRegistered() && It->HasBegunPlay())
			{
				return *It;
			}
		}

		if (!World->IsGameWorld() || !World->GetFirstPlayerController())
		{
			ScreenMessage(TEXT("[Dialogos] Dale a Play antes de usar los comandos de dialogo."), FColor::Red);
			return nullptr;
		}

		AActor* Host = World->SpawnActor<AActor>();
		UEgnisDialogueComponent* Comp = NewObject<UEgnisDialogueComponent>(Host, TEXT("EgnisDialogue"));
		Comp->bStartOnPlayerOverlap = false;
		Comp->RegisterComponent();
		Host->AddInstanceComponent(Comp);
		return Comp;
	}

	static void CmdStart(const TArray<FString>& Args, UWorld* World)
	{
		if (UEgnisDialogueComponent* Comp = FindOrCreate(World))
		{
			Comp->StartDialogue(Args.Num() > 0 ? FName(*Args[0]) : NAME_None);
		}
	}

	static void CmdFlag(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() == 0)
		{
			ScreenMessage(TEXT("Uso: Egnis.DialogueFlag NombreFlag 1|0"), FColor::Yellow);
			return;
		}
		const bool bValue = Args.Num() < 2 || Args[1].ToBool() || Args[1] == TEXT("1");
		for (TObjectIterator<UEgnisDialogueComponent> It; It; ++It)
		{
			if (It->GetWorld() == World)
			{
				It->SetFlag(FName(*Args[0]), bValue);
			}
		}
		if (UEgnisDialogueComponent* Comp = FindOrCreate(World))
		{
			Comp->SetFlag(FName(*Args[0]), bValue);
		}
		ScreenMessage(FString::Printf(TEXT("[Dialogos] Flag %s = %s"), *Args[0], bValue ? TEXT("true") : TEXT("false")), FColor::Green);
	}

	static void CmdLang(const TArray<FString>& Args, UWorld* World)
	{
		const bool bSpanish = Args.Num() == 0 || !Args[0].Equals(TEXT("en"), ESearchCase::IgnoreCase);
		for (TObjectIterator<UEgnisDialogueComponent> It; It; ++It)
		{
			if (It->GetWorld() == World)
			{
				It->bUseSpanish = bSpanish;
			}
		}
		ScreenMessage(FString::Printf(TEXT("[Dialogos] Idioma: %s"), bSpanish ? TEXT("es") : TEXT("en")), FColor::Green);
	}

	static void CmdReload(const TArray<FString>& Args, UWorld* World)
	{
		for (TObjectIterator<UEgnisDialogueComponent> It; It; ++It)
		{
			if (It->GetWorld() == World && It->HasBegunPlay())
			{
				It->ReloadData();
			}
		}
		ScreenMessage(TEXT("[Dialogos] Recargando el Excel..."), FColor::Green);
	}

	static void CmdEnd(const TArray<FString>& Args, UWorld* World)
	{
		for (TObjectIterator<UEgnisDialogueComponent> It; It; ++It)
		{
			if (It->GetWorld() == World)
			{
				It->EndDialogue();
			}
		}
	}

	static void CmdList(const TArray<FString>& Args, UWorld* World)
	{
		UEgnisDialogueComponent* Comp = FindOrCreate(World);
		if (!Comp)
		{
			return;
		}
		if (!Comp->IsDataLoaded())
		{
			ScreenMessage(TEXT("[Dialogos] Aun cargando, prueba en un segundo."), FColor::Yellow);
			return;
		}
		const FString Filter = Args.Num() > 0 ? Args[0] : FString();
		int32 Shown = 0;
		for (const FName& Id : Comp->GetDialogueEntryPoints())
		{
			if (!Filter.IsEmpty() && !Id.ToString().Contains(Filter))
			{
				continue;
			}
			UE_LOG(LogEgnisDialogue, Display, TEXT("  %s"), *Id.ToString());
			if (Shown < 40)
			{
				ScreenMessage(Id.ToString(), FColor::White, 15.f);
			}
			++Shown;
		}
		ScreenMessage(FString::Printf(TEXT("[Dialogos] %d inicios de dialogo (lista completa en el Output Log):"), Shown), FColor::Green, 15.f);
	}

	static FAutoConsoleCommandWithWorldAndArgs GCmdStart(TEXT("Egnis.Dialogue"),
		TEXT("Empieza un dialogo: Egnis.Dialogue <Row_ID>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdStart));
	static FAutoConsoleCommandWithWorldAndArgs GCmdFlag(TEXT("Egnis.DialogueFlag"),
		TEXT("Activa/desactiva un flag de Conditions: Egnis.DialogueFlag <Flag> 1|0"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdFlag));
	static FAutoConsoleCommandWithWorldAndArgs GCmdLang(TEXT("Egnis.DialogueLang"),
		TEXT("Idioma de los dialogos: Egnis.DialogueLang es|en"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdLang));
	static FAutoConsoleCommandWithWorldAndArgs GCmdReload(TEXT("Egnis.DialogueReload"),
		TEXT("Vuelve a descargar el Excel de dialogos"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdReload));
	static FAutoConsoleCommandWithWorldAndArgs GCmdEnd(TEXT("Egnis.DialogueEnd"),
		TEXT("Cierra el dialogo actual"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdEnd));
	static FAutoConsoleCommandWithWorldAndArgs GCmdList(TEXT("Egnis.DialogueList"),
		TEXT("Lista los Row_ID donde empieza cada dialogo: Egnis.DialogueList [filtro]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdList));
}
