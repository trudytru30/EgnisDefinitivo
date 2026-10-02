#include "Characters/BoardPlayerController.h"
#include "CoreAndSystems/AudioManager.h"
#include "Cards/BaseCard.h"
#include "CoreAndSystems/BattleManager.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "DrawDebugHelpers.h"
#include "CoreAndSystems/Board.h"
#include "Characters/CharacterBase.h"
#include "Cards/DeckManager.h"
#include "CoreAndSystems/GameManager.h"
#include "Characters/Ally.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/HealthAttributeSet.h"

ABoardPlayerController::ABoardPlayerController()
{
    bShowMouseCursor = true;
}

void ABoardPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
       if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
       {
          if (IMCGameplay)
             Subsystem->AddMappingContext(IMCGameplay, 0);
          if (IMCUI)
             Subsystem->AddMappingContext(IMCUI, 1);
       }
    }

    if (AGameManager* GM = Cast<AGameManager>(GetWorld()->GetAuthGameMode()))
    {
       BM = GM->GetBattleManager();
       DeckManager = GM->GetDeckManager();
        BSM = GM->BattleSceneManager;
    }

    if (BM)
       BM->OnPlayerTurnStarted.AddDynamic(this, &ABoardPlayerController::BP_RefreshHandUI);
    else
       UE_LOG(LogTemp, Error, TEXT("[BoardPlayerController]: BattleManager is null"));

    if (GameHUDClass)
    {
       HUDWidget = CreateWidget<UUserWidget>(this, GameHUDClass);
       if (HUDWidget)
          HUDWidget->AddToViewport();
    }
    BP_RefreshHandUI();
}

void ABoardPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput) return;

    if (!ClickAction || !PauseAction)
    {
       UE_LOG(LogTemp, Warning, TEXT("ClickAction or PauseAction not found"));
       return;
    }
    
    EnhancedInput->BindAction(ClickAction, ETriggerEvent::Started, this, &ABoardPlayerController::HandleLeftClick);
    EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &ABoardPlayerController::HandleMenu);
}

// ==========================================
// ====== REFACTORIZACIÓN DE INPUTS ======
// ==========================================

void ABoardPlayerController::HandleLeftClick()
{
    if (bIsInMenu) return;

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("IA_Click triggered"));

    // Delegamos según el estado actual
    if (SelectionState != ECardSelectionState::None)
    {
        ProcessCardStateInput();
    }
    else
    {
        ProcessDefaultStateInput();
    }
}

void ABoardPlayerController::ProcessCardStateInput()
{
    if (SelectionState == ECardSelectionState::SelectingUnit)
    {
        TrySelectCardSource();
    }
    else if (SelectionState == ECardSelectionState::SelectingTarget)
    {
        TrySelectCardTarget();
    }
}

void ABoardPlayerController::ProcessDefaultStateInput()
{
    FHitResult Hit;
    if (TraceUnderCursor(BoardTraceChannel, Hit))
    {
        FVector FinalLocation = Hit.ImpactPoint;
        FRotator FixedRotation = FRotator(0.f, 0.f, 0.f); // Tu rotación fija

        // Centramos en la baldosa exacta y subimos un poco la Z para que no roce el suelo
        if (ABoard* Board = Cast<ABoard>(Hit.GetActor()))
        {
            FTileCoord Tile;
            if (Board->WorldPointToTile(Hit.ImpactPoint, Tile))
            {
                FinalLocation = Board->TileToWorldCenter(Tile);
                FinalLocation.Z = 1.f; // Margen de altura
            }
        }

        // Ejecutamos el despliegue con la posición limpia y centrada
        BSM->TryDeployHeroAtLocation(FinalLocation, FixedRotation);
        return; 
    }

    // 2. MODO NORMAL: Intentar seleccionar un aliado (si se selecciona, terminamos)
    if (TrySelectAlly()) 
    {
        return; 
    }

    // 3. MODO NORMAL: Si no seleccionó nada nuevo, intentar mover al aliado que ya tenía
    if (CurrentIntent == EInputIntent::Move)
    {
        TryMoveSelectedAlly();
    }
}

void ABoardPlayerController::TrySelectCardSource()
{
    FHitResult Hit;
    if (!TraceUnderCursor(UnitTraceChannel, Hit)) return;

    AAlly* ClickedAlly = Cast<AAlly>(Hit.GetActor());
    if (!ClickedAlly)
    {
        UE_LOG(LogTemp, Warning, TEXT("BoardPlayerComponent: Clicked actor is not an ally"));
        return;
    }

    // Validación de arquetipo
    if (PendingCard && PendingCard->GetColor() != EColorType::Grey && !ClickedAlly->HasArchetypeColor(PendingCard->GetColor()))
    {
        UE_LOG(LogTemp, Warning, TEXT("BoardPlayerController: %s no puede jugar una carta de arquetipo distinto"), *ClickedAlly->GetName());
        return;
    }

    PendingSource = ClickedAlly;

    // Si no necesita target, la jugamos inmediatamente
    if (PendingCardTarget == ECardTarget::None || PendingCardTarget == ECardTarget::Self)
    {
        ExecuteCardPlay(nullptr, FVector::ZeroVector);
        return;
    }

    // Si necesita target, pasamos a la siguiente fase
    SelectionState = ECardSelectionState::SelectingTarget;
}

void ABoardPlayerController::TrySelectCardTarget()
{
    FHitResult Hit;
    ACharacterBase* TargetUnit = nullptr;
    FVector TargetLocation = FVector::ZeroVector;

    UE_LOG(LogTemp, Warning, TEXT("[SelectingTarget] PendingTarget=%d"), (int32)PendingCardTarget);

    if (PendingCardTarget == ECardTarget::Enemy)
    {
        if (!TraceUnderCursor(UnitTraceChannel, Hit)) return;
        TargetUnit = Cast<ACharacterBase>(Hit.GetActor());
        if (!TargetUnit || TargetUnit->GetTeam() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Selected target is not an enemy."));
            return;
        }
    }
    else if (PendingCardTarget == ECardTarget::Ally)
    {
        if (!TraceUnderCursor(UnitTraceChannel, Hit)) return;
        TargetUnit = Cast<ACharacterBase>(Hit.GetActor());
        if (!TargetUnit || TargetUnit->GetTeam() != 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Selected target is not an ally."));
            return;
        }
    }
    else if (PendingCardTarget == ECardTarget::Tile)
    {
        if (!TraceUnderCursor(BoardTraceChannel, Hit)) return;
        TargetLocation = Hit.ImpactPoint;
    }

    // Comprobación de que no se elige a la misma unidad como objetivo si no es Self
    if (TargetUnit == PendingSource)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target cannot be the source"));
        return;
    }

    ExecuteCardPlay(TargetUnit, TargetLocation);
}

void ABoardPlayerController::ExecuteCardPlay(ACharacterBase* TargetUnit, FVector TargetLocation)
{
    if (!PendingCard || !PendingSource || !BM) return;

    BM->PlayCard(PendingCard, PendingSource, TargetUnit, TargetLocation);
    UE_LOG(LogTemp, Warning, TEXT("PlayCard ejecutada"));
    
    if (DeckManager)
    {
        DeckManager->DiscardCardFromHand(PendingCard);
        UE_LOG(LogTemp, Warning, TEXT("Cartas en mano tras descartar: %d"), DeckManager->GetHand().Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DeckManager NULL al descartar"));
    }

    BP_RefreshHandUI();

    // Resetear estados
    PendingCard = nullptr;
    PendingSource = nullptr;
    SelectionState = ECardSelectionState::None;
    CurrentIntent = EInputIntent::Move;
}

bool ABoardPlayerController::TrySelectAlly()
{
    FHitResult Hit;
    if (TraceUnderCursor(UnitTraceChannel, Hit))
    {
        if (ACharacterBase* ClickedCharacter = Cast<ACharacterBase>(Hit.GetActor()))
        {
            if (ClickedCharacter->GetTeam() == 0) // Es Aliado
            {
                SelectedAlly = ClickedCharacter;
                if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayElevateSound(0);
                
                if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, FString::Printf(TEXT("Selected: %s"), *SelectedAlly->GetName()));
                UE_LOG(LogTemp, Log, TEXT("Selected Ally: %s"), *SelectedAlly->GetName());
                return true;
            }
        }
    }
    return false;
}

void ABoardPlayerController::TryMoveSelectedAlly()
{
    FHitResult Hit;
    if (!TraceUnderCursor(BoardTraceChannel, Hit)) return;

    if (ABoard* Board = Cast<ABoard>(Hit.GetActor()))
    {
        FTileCoord Tile;
        if (Board->WorldPointToTile(Hit.ImpactPoint, Tile))
        {
            if (!SelectedAlly)
            {
                if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("No ally selected"));
                return;
            }

            if (!BM) return;

            BM->RequestMove(SelectedAlly, Tile);

            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Tile = (%d, %d)"), Tile.X, Tile.Y));
            
            // Construir el Result para enviar al Blueprint (Efectos visuales)
            FClickResult Result;
            Result.bHit = true; 
            Result.bHitBoard = true;
            Result.HitActor = Board; 
            Result.WorldPoint = Hit.ImpactPoint;
            BP_OnclickResolved(Result);
            
            DrawDebugSphere(GetWorld(), Result.WorldPoint, 10.f, 12, FColor::Green, false, 1.0f);
        }
        else
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Click fuera del tablero"));
        }
    }
}

// ==========================================
// ======== RESTO DE LAS FUNCIONES ========
// ==========================================

bool ABoardPlayerController::TraceUnderCursor(ECollisionChannel Channel, FHitResult& OutHit) const
{
    FVector WorldOrigin, WorldDirection;
    if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
    {
       return false;
    }

    const float TraceDistance = 200000.f;
    const FVector Start = WorldOrigin;
    const FVector End = WorldOrigin + (WorldDirection * TraceDistance);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(ClickTrace), true);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);

    if (bHit)
    {
       DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 12.f, 12, FColor::Green, false, 2.0f);
    }

    return bHit;
}

void ABoardPlayerController::OpenMenu()
{
    bIsInMenu = true;

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
       if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
       {
          Subsystem->RemoveMappingContext(IMCGameplay);
       }
    }

    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}

void ABoardPlayerController::CloseMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("CloseMenu"));
    bIsInMenu = false;

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
       if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
       {
          Subsystem->AddMappingContext(IMCGameplay, 0);
       }
    }

    SetInputMode(FInputModeGameAndUI());
}

void ABoardPlayerController::HandleMenu()
{
    bIsInMenu ? CloseMenu() : OpenMenu();
}

void ABoardPlayerController::BeginPlayCard(UBaseCard* Card)
{
    UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: BeginPlayCard llamada con carta: %s"), Card ? *Card->GetName() : TEXT("NULL"));
    if (!Card) return;

    if (!BM || !BM->IsPlayerTurn())
    {
       UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: Cannot play card: Not player turn"));
       return;
    }

    if (SelectionState != ECardSelectionState::None && PendingCard == Card)
    {
       PendingCard = nullptr;
       PendingSource = nullptr;
       SelectionState = ECardSelectionState::None;
       CurrentIntent = EInputIntent::Move;
       BP_RefreshHandUI();
       return;
    }

    PendingCard = Card;
    PendingSource = nullptr;
    PendingCardTarget = Card->GetTarget();
    SelectionState = ECardSelectionState::SelectingUnit;
    CurrentIntent = EInputIntent::Action; 
    BP_RefreshHandUI();
}

void ABoardPlayerController::RequestEndTurn()
{
    if (!BM || !BM->IsPlayerTurn())
    {
       UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: Cannot end turn: Not player turn"));
       return;
    } else if (SelectionState != ECardSelectionState::None)
    {
       UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: Cancelling card selection and ending turn..."));
       PendingCard = nullptr;
       PendingSource = nullptr;
       SelectionState = ECardSelectionState::None;
       CurrentIntent = EInputIntent::Move;
    }

    BM->EndTurn();
}

TArray<UBaseCard*> ABoardPlayerController::GetCurrentHand() const
{
    if (DeckManager)
    {
       UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: GetCurrentHand: %d cartas"), DeckManager->GetHand().Num());
       return DeckManager->GetHand();
    }

    UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: GetCurrentHand: DeckManager NULL"));
    return TArray<UBaseCard*>();
}

bool ABoardPlayerController::GetPendingCardRangeTiles(TArray<FTileCoord>& OutTiles) const
{
    OutTiles.Reset();

    if (!PendingCard || !PendingSource || !PendingSource->Board)
    {
       return false;
    }

    PendingSource->Board->GetTilesInRange(PendingSource->CurrentTile, PendingCard->GetRange(), OutTiles);
    return true;
}

bool ABoardPlayerController::IsCardArchetypeAvailable(const UBaseCard* Card) const
{
    if (!DeckManager || !Card) return false;

    TArray<AActor*> AllyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAlly::StaticClass(), AllyActors);

    TArray<ACharacterBase*> CharactersInPlay;
    for (AActor* Actor : AllyActors)
    {
       if (ACharacterBase* CharBase = Cast<ACharacterBase>(Actor))
       {
          CharactersInPlay.Add(CharBase);
       }
    }

    return DeckManager->IsCardArchetypeAvailable(Card, CharactersInPlay);
}

void ABoardPlayerController::DebugDamageSelectedUnit(float Amount)
{
    if (!SelectedAlly)
    {
       UE_LOG(LogTemp, Warning, TEXT("[Debug] No hay ninguna unidad seleccionada."));
       return;
    }

    UAbilitySystemComponent* ASC = SelectedAlly->GetAbilitySystemComponent();
    if (!ASC || !DebugDamageEffect)
    {
       UE_LOG(LogTemp, Warning, TEXT("[Debug] Falta ASC o DebugDamageEffect (asígnalo en los defaults del BP)."));
       return;
    }

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DebugDamageEffect, 1.f, Context);
    if (Spec.IsValid())
    {
       ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }

    UE_LOG(LogTemp, Log, TEXT("[Debug] GE aplicado a %s. Vida restante: %.0f"),
       *SelectedAlly->GetName(), SelectedAlly->HealthAttributeSet->GetHealth());
}