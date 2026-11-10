#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dialogue/EgnisDialogueTypes.h"
#include "EgnisDialogueComponent.generated.h"

class UEgnisDialogueWidget;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEgnisDialogueStartedSignature, FName, StartRowID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEgnisDialogueEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEgnisDialogueLineSignature, const FEgnisDialogueLine&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEgnisDialogueActionSignature, const FString&, Action, FName, RowID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEgnisDialogueAudioCueSignature, const FString&, AudioCue, FName, RowID);

/**
 * Sistema de diálogos que lee el Excel de diálogos de Egnis
 * (columnas Row_ID, Next_Row_ID, Character_ID, Type, Parent_Row_ID, Chance,
 *  Conditions, Audio_Cue, Actions, Text_en, Text_es).
 *
 *  - Line:    se muestra y se pasa a Next_Row_ID (vacío o END = fin).
 *  - Choice:  opciones del jugador; cuelgan de la línea indicada en Parent_Row_ID.
 *  - Random:  elige al azar una de sus Variant (las filas cuyo Parent_Row_ID es el Random),
 *             usando Chance como peso (vacío = 1).
 *  - Conditions: nombres de flags separados por '&', con '!' para negar.
 *
 * Los datos se descargan del Google Sheet al empezar (y se guardan en Saved/DialogueCache
 * para poder jugar sin conexión). También acepta DataTables de FGameTextRow.
 */
UCLASS(ClassGroup = (Egnis), meta = (BlueprintSpawnableComponent, DisplayName = "Egnis Dialogue"))
class EGNISDEF_API UEgnisDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEgnisDialogueComponent();

	// ---------- Configuración ----------

	/** Fila con la que empieza el diálogo de este actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName StartRowID = TEXT("Ghol_Phase01_Greet_01");

	/** Empieza el diálogo cuando el pawn del jugador entra en el actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bStartOnPlayerOverlap = true;

	/** Empieza el diálogo nada más empezar la partida (útil para probar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bStartOnBeginPlay = false;

	/** Si el actor no tiene ninguna colisión, crea una esfera trigger de este radio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	float TriggerRadius = 250.f;

	/** Sólo se puede lanzar una vez por partida. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bOnlyOnce = false;

	/** true = español (Text_es), false = inglés (Text_en). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bUseSpanish = true;

	/** Descarga las hojas del Google Sheet al empezar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Data")
	bool bDownloadFromGoogleSheets = true;

	/** URLs de exportación CSV de cada pestaña del Excel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Data")
	TArray<FString> GoogleSheetCsvUrls;

	/** Opcional: DataTables (struct GameTextRow) sincronizadas con Sheets Importer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Data")
	TArray<TObjectPtr<UDataTable>> DialogueTables;

	/** Flags que se usan en la columna Conditions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Conditions")
	TMap<FName, bool> Flags;

	/** Widget a usar. Si está vacío se usa el widget por defecto (generado por código). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|UI")
	TSubclassOf<UEgnisDialogueWidget> WidgetClass;

	/** Muestra en pantalla las Actions y Audio_Cue (para depurar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Debug")
	bool bPrintActionsOnScreen = true;

	// ---------- Eventos ----------

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FEgnisDialogueStartedSignature OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FEgnisDialogueEndedSignature OnDialogueEnded;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FEgnisDialogueLineSignature OnLineShown;

	/** Se lanza con el contenido de la columna Actions (p.ej. "ReceiveEgnisBlessing"). */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FEgnisDialogueActionSignature OnDialogueAction;

	/** Se lanza con el contenido de la columna Audio_Cue (evento de FMOD). */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FEgnisDialogueAudioCueSignature OnAudioCue;

	// ---------- API ----------

	/** Empieza un diálogo desde la fila indicada (None = StartRowID). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogue(FName RowID = NAME_None);

	/** Pasa a la siguiente línea (si no hay opciones en pantalla). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void Advance();

	/** Elige una de las opciones mostradas (0 = la primera). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SelectChoice(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SetFlag(FName Flag, bool bValue);

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return bActive; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDataLoaded() const { return bDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	const FEgnisDialogueLine& GetCurrentLine() const { return CurrentLine; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool FindRow(FName RowID, FEgnisDialogueRow& OutRow) const;

	/** Vuelve a cargar los datos (descarga de nuevo el Excel). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ReloadData();

	/** Filas que parecen inicios de diálogo (nadie apunta a ellas). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	TArray<FName> GetDialogueEntryPoints() const;

	int32 GetNumRows() const { return Rows.Num(); }

	/** Parser de CSV (soporta comillas, comas y saltos de línea dentro de celdas). */
	static void ParseCsv(const FString& Csv, TArray<TArray<FString>>& OutRows);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleOwnerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

private:
	void LoadFromDataTables();
	void LoadFromCsv(const FString& Csv, const FString& SheetName);
	void AddRow(const FEgnisDialogueRow& Row);
	void OnAllSourcesLoaded();
	FString GetCachePath(int32 Index) const;

	/** Resuelve RowID (Random, condiciones, etc.) y lo muestra. */
	void GoTo(FName RowID, int32 Depth = 0);
	void ShowLine(const FEgnisDialogueRow& Row);
	void GatherChoices(const FEgnisDialogueRow& Row, TArray<int32>& OutChoiceIndices) const;
	int32 PickVariant(const FEgnisDialogueRow& RandomRow) const;
	bool EvaluateConditions(const FString& Conditions) const;
	FText GetRowText(const FEgnisDialogueRow& Row) const;
	FText GetSpeakerText(const FEgnisDialogueRow& Row) const;
	void FireRowEvents(const FEgnisDialogueRow& Row);

	int32 FindRowIndex(FName RowID) const;
	void CreateTriggerIfNeeded();
	void EnsureWidget();
	void SetUIInput(bool bEnable);

	static bool IsEndToken(FName RowID);

	TArray<FEgnisDialogueRow> Rows;
	TMap<FName, int32> RowIndexByID;

	UPROPERTY(Transient)
	TObjectPtr<UEgnisDialogueWidget> Widget;

	FEgnisDialogueLine CurrentLine;
	TArray<FName> CurrentChoiceTargets;
	FName CurrentNextRowID;

	bool bActive = false;
	bool bDataLoaded = false;
	bool bAlreadyPlayed = false;
	bool bPreviousShowCursor = false;
	int32 PendingDownloads = 0;
	int32 LoadGeneration = 0;
	FName PendingStartRow;
	bool bHasPendingStart = false;
};
