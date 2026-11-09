#pragma once

#include "CoreMinimal.h"
#include "Cards/CardTarget.h"
#include "Cards/DeckManager.h"
#include "GameFramework/PlayerController.h"
#include "BoardPlayerController.generated.h"

class UAudioManager;
class UInteractableComponent;

class AAlly;
class UBaseCard;
class ACharacterBase;
class UInputAction;
class UInputMappingContext;
class UBattleManager;
class UDeckManager;
class UBattleSceneManager;
class UDeployManager;

#pragma region Enums
UENUM(BlueprintType)
enum class EInputIntent: uint8
{
    Move,
    Action
};

UENUM(BlueprintType)
enum class ECardSelectionState: uint8
{
    None,
    SelectingUnit,
    SelectingTarget
};

#pragma endregion

USTRUCT(BlueprintType)
struct FClickResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) bool  bHit = false;
    UPROPERTY(BlueprintReadWrite) bool  bHitUnit = false;
    UPROPERTY(BlueprintReadWrite) bool  bHitBoard = false;

    UPROPERTY(BlueprintReadWrite) TObjectPtr<class AActor> HitActor = nullptr;
    UPROPERTY(BlueprintReadWrite) FVector WorldPoint = FVector::ZeroVector;
};

UCLASS()
class EGNISDEF_API ABoardPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ABoardPlayerController();

    // PROPIEDAD TEMPORAL
    UPROPERTY(EditDefaultsOnly, Category="Debug")
    TSubclassOf<class UGameplayEffect> DebugDamageEffect;
    
    // TEMPORAL: para probar de forma aislada que el daño vía ASC dispara la muerte por GAS
    // sin depender de que las cartas ya usen abilities.
    UFUNCTION(Exec)
    void DebugDamageSelectedUnit(float Amount = 150.f);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    EInputIntent CurrentIntent = EInputIntent::Move;

    //Getter para UI
    UFUNCTION(BlueprintCallable, Category="Selection")
    ACharacterBase* GetSelectedAlly() const { return SelectedAlly; }

    UFUNCTION(BlueprintPure, Category="Cards")
    bool IsCardArchetypeAvailable(const UBaseCard* Card) const;

    UFUNCTION(BlueprintCallable, Category="Battle|Turn")
    void RequestEndTurn();

    UFUNCTION(BlueprintCallable, Category="Battle|Cards")
    void BeginPlayCard(UBaseCard* Card);

    UFUNCTION(BlueprintCallable, Category="Battle|Cards")
    void TryPlayCardAtCursor(UBaseCard* Card);

    UFUNCTION(BlueprintCallable, Category="Battle|Cards")
    void BeginCardDrag(UBaseCard* Card);

    UFUNCTION(BlueprintCallable, Category="Battle|Cards")
    void CancelPendingCard();

    UFUNCTION(BlueprintPure, Category="Battle|Cards")
    TArray<UBaseCard*> GetCurrentHand() const;

    UFUNCTION(BlueprintCallable, Category="Battle|Cards")
    bool GetPendingCardRangeTiles(TArray<FTileCoord>& OutTiles) const;

    UFUNCTION(BlueprintImplementableEvent, Category="Battle|UI")
    void BP_RefreshHandUI();
    
    // SOLO PARA PRUEBAS (consola del editor): confirma la seleccion de cartas sin necesitar el boton de la UI.
    // TODO: quitar cuando la UI tenga el boton Confirmar.
    UFUNCTION(Exec)
    void DebugConfirmHandSelection();
    
    UPROPERTY()
    UDeckManager* DeckManager = nullptr;

protected:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupInputComponent() override;

    //Ally seleccionado
    UPROPERTY()
    TObjectPtr<ACharacterBase> SelectedAlly = nullptr;

    // Carta seleccionada (para UI)
    UPROPERTY(BlueprintReadOnly, Category = "Cards")
    TObjectPtr<UBaseCard> PendingCard = nullptr;

#pragma region Inputs
    UFUNCTION(BlueprintImplementableEvent, Category="Input")
    void BP_OnclickResolved(const FClickResult& Result);

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputMappingContext> IMCGameplay;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputMappingContext> IMCUI;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputAction> PauseAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ClickAction ;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> RightClickAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditDefaultsOnly, Category="Input|Ally Selection")
    TObjectPtr<UInputAction> SelectAlly1Action;

    UPROPERTY(EditDefaultsOnly, Category="Input|Ally Selection")
    TObjectPtr<UInputAction> SelectAlly2Action;

    UPROPERTY(EditDefaultsOnly, Category="Input|Ally Selection")
    TObjectPtr<UInputAction> SelectAlly3Action;

    UPROPERTY(EditDefaultsOnly, Category="Input|Ally Selection")
    TObjectPtr<UInputAction> SelectAlly4Action;

    UPROPERTY(EditDefaultsOnly, Category="Input|Ally Selection")
    TObjectPtr<UInputAction> PreviousAllyAction;

    UPROPERTY(EditDefaultsOnly, Category="Input|Ally Selection")
    TObjectPtr<UInputAction> NextAllyAction;

    // Actualiza el marcado del Blueprint sin ejecutar su trace de clic bajo el cursor.
    UFUNCTION(BlueprintImplementableEvent, Category="Selection")
    void BP_OnKeyboardAllySelected(ACharacterBase* Ally);
#pragma endregion

    UPROPERTY(EditDefaultsOnly, Category="Click|Trace")
    TEnumAsByte<ECollisionChannel> UnitTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditDefaultsOnly, Category="Click|Trace")
    TEnumAsByte<ECollisionChannel> BoardTraceChannel = ECC_GameTraceChannel2;

private:

    AAlly* GetSelectedCaster(const UBaseCard* Card) const;
    bool ResolveCardTargetUnderCursor(ACharacterBase*& OutTargetUnit, FVector& OutTargetLocation) const;
    bool DeprojectCursorToWorld(FVector& OutOrigin, FVector& OutDirection) const;
    
    // Si hay una seleccion de cartas en curso, el clic en una carta la marca o desmarca en vez de jugarla.
    // Devuelve true si estaba seleccionando (y por tanto el clic ya esta gestionado).
    bool HandleCardClickWhileSelecting(UBaseCard* Card);

#pragma region Variables
    UPROPERTY()
    TObjectPtr<UBattleManager> BM = nullptr;

    UPROPERTY()
    TObjectPtr<UBattleSceneManager> BSM = nullptr;

    UPROPERTY()
    TObjectPtr<UDeployManager> DPM = nullptr;

    bool bIsInMenu = false;

    TWeakObjectPtr<UInteractableComponent> HoveredInteractable;
    void UpdateHoveredInteractable();

    UPROPERTY(EditDefaultsOnly, Category="Interaction")
    TEnumAsByte<ECollisionChannel> InteractionTraceChannel = ECC_Visibility;

    //Cartas
    UPROPERTY()
    TObjectPtr<AAlly> PendingSource = nullptr;
    ECardSelectionState SelectionState = ECardSelectionState::None;
    UPROPERTY()
    ECardTarget PendingCardTarget = ECardTarget::None; // Target de la carta que se quiere jugar
#pragma endregion

#pragma region Functions
    void OpenMenu();
    void CloseMenu();
    void HandleMenu();
    
    // Funciones principales de Input
    void HandleLeftClick();
    void HandleRightClick();
    void HandleInteract();
    bool TraceUnderCursor(ECollisionChannel Channel, FHitResult& OutHit ) const;

    // --- NUEVAS FUNCIONES DE REFACTORIZACIÓN ---
    void ProcessCardStateInput();
    void ProcessDefaultStateInput();
    void TrySelectCardTarget();
    bool TrySelectAlly();
    void SelectAlly(ACharacterBase* Ally);
    bool CanSelectAllyFromInput() const;
    // Indice base 0: las teclas 1-4 usaran los indices 0-3 de DeployedHeroes.
    bool SelectAllyByIndex(int32 Index);
    void HandleSelectAllyByIndex(int32 Index);
    // Direction: -1 anterior, +1 siguiente. No vuelve al otro extremo del array.
    void SelectAdjacentAlly(int32 Direction);
    void ExecuteCardPlay(ACharacterBase* TargetUnit, FVector TargetLocation);
    // -------------------------------------------
#pragma endregion
};
