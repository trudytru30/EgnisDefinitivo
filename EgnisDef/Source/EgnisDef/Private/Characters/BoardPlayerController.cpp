#include "Characters/BoardPlayerController.h"
#include "Components/InteractableComponent.h"
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
#include "CoreAndSystems/DeployManager.h"
#include "CoreAndSystems/HealthAttributeSet.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Environment/ObstacleBase.h"

ABoardPlayerController::ABoardPlayerController()
{
    bShowMouseCursor = true;
}

void ABoardPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (IsLocalController()) UpdateHoveredInteractable();
}

void ABoardPlayerController::UpdateHoveredInteractable()
{
    UInteractableComponent* Next = nullptr;
    FHitResult Hit;
    if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(InteractionTraceChannel), false, Hit))
    {
        if (AActor* Actor = Hit.GetActor()) Next = Actor->FindComponentByClass<UInteractableComponent>();
    }
    if (HoveredInteractable.Get() == Next) return;
    if (HoveredInteractable.IsValid()) HoveredInteractable->SetHighlighted(false);
    HoveredInteractable = Next;
    if (Next) Next->SetHighlighted(true);
}

void ABoardPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HoveredInteractable.IsValid()) HoveredInteractable->SetHighlighted(false);
    HoveredInteractable.Reset();
    Super::EndPlay(EndPlayReason);
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
        
        BSM = GM->GetBattleSceneManager();
        DPM = BSM->GetDeployManager();
        BM = BSM->GetBattleManager();
        DeckManager = BSM->GetDeckManager();
    }

    if (BM)
        BM->OnPlayerTurnStarted.AddDynamic(this, &ABoardPlayerController::BP_RefreshHandUI);
    else
        UE_LOG(LogTemp, Error, TEXT("[BoardPlayerController]: BattleManager is null"));

    if (DeckManager)
    {
        DeckManager->OnHandChanged.AddDynamic(this, &ABoardPlayerController::BP_RefreshHandUI);
    }
}

void ABoardPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput) return;

    if (InteractAction)
    {
        EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ABoardPlayerController::HandleInteract);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InteractAction not assigned"));
    }
    if (!ClickAction || !PauseAction)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClickAction or PauseAction not found"));
        return;
    }
    
    EnhancedInput->BindAction(ClickAction, ETriggerEvent::Started, this, &ABoardPlayerController::HandleLeftClick);
    EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &ABoardPlayerController::HandleMenu);

    if (RightClickAction)
    {
        EnhancedInput->BindAction(RightClickAction, ETriggerEvent::Started, this, &ABoardPlayerController::HandleRightClick);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RightClickAction not found: no se podrán retirar héroes en el despliegue"));
    }
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

void ABoardPlayerController::HandleInteract()
{
    if (!IsLocalController()) return;

    UpdateHoveredInteractable();
    if (HoveredInteractable.IsValid())
    {
        HoveredInteractable->Interact();
    }
}
void ABoardPlayerController::HandleRightClick()
{
    if (bIsInMenu) return;

    if (!BSM || BSM->CurrentPhase != EScenePhase::Deployment || !DPM) return;

    FHitResult Hit;
    if (!TraceUnderCursor(UnitTraceChannel, Hit)) return;

    AAlly* ClickedAlly = Cast<AAlly>(Hit.GetActor());
    if (ClickedAlly && DPM->RemoveDeployedHero(ClickedAlly))
    {
        //el actor se está destruyendo: que no se quede como seleccionado
        if (SelectedAlly == ClickedAlly)
        {
            SelectedAlly = nullptr;
        }
    }
}

void ABoardPlayerController::ProcessCardStateInput()
{
    if (SelectionState == ECardSelectionState::SelectingTarget)
    {
        TrySelectCardTarget();
    }
}

void ABoardPlayerController::ProcessDefaultStateInput()
{
    if (TrySelectAlly()) 
    {
        return; 
    }

    //Comprobar si has hecho click en el tablero
    FHitResult Hit;
    if (TraceUnderCursor(BoardTraceChannel, Hit))
    {
        //Si está en fase de despliegue intentará colocar un personaje
        if (BSM && BSM->CurrentPhase == EScenePhase::Deployment)
        {
            ABoard* Board = Cast<ABoard>(Hit.GetActor());
            FTileCoord Tile;
            if (DPM && Board && Board->WorldPointToTile(Hit.ImpactPoint, Tile))
            {
                DPM->TryDeployHeroAtTile(Board, Tile);
            }
            return;
        }
        
        //Si no está en fase de despliegue se intenta mover
        if (CurrentIntent == EInputIntent::Move && SelectedAlly)
        {
            TryMoveSelectedAlly(Hit);
        }
    }
}

void ABoardPlayerController::TrySelectCardTarget()
{
    UE_LOG(LogTemp, Warning, TEXT("[SelectingTarget] PendingTarget=%d"), (int32)PendingCardTarget);

    ACharacterBase* TargetUnit = nullptr;
    FVector TargetLocation = FVector::ZeroVector;
    if (ResolveCardTargetUnderCursor(TargetUnit, TargetLocation))
    {
        ExecuteCardPlay(TargetUnit, TargetLocation);
    }
}

bool ABoardPlayerController::ResolveCardTargetUnderCursor(ACharacterBase*& OutTargetUnit, FVector& OutTargetLocation) const
{
    FHitResult Hit;
    OutTargetUnit = nullptr;
    OutTargetLocation = FVector::ZeroVector;

    if (PendingCardTarget == ECardTarget::Enemy)
    {
        if (!TraceUnderCursor(UnitTraceChannel, Hit)) return false;
        
        // Obstaculo destructible: solo si algun efecto de la carta puede golpearlo. Viaja por Location, sin unidad
        AObstacleBase* Obstacle = Cast<AObstacleBase>(Hit.GetActor());
        if (Obstacle && Obstacle->bDestructible && PendingCard && PendingCard->CanTargetObstacle())
        {
            OutTargetLocation = Obstacle->GetActorLocation();
        } 
        else
        {
            OutTargetUnit = Cast<ACharacterBase>(Hit.GetActor());
            if (!OutTargetUnit || OutTargetUnit->GetTeam() == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("Selected target is not an enemy."));
                return false;
            }
        }
    }
    else if (PendingCardTarget == ECardTarget::Ally)
    {
        if (!TraceUnderCursor(UnitTraceChannel, Hit)) return false;
        OutTargetUnit = Cast<ACharacterBase>(Hit.GetActor());
        if (!OutTargetUnit || OutTargetUnit->GetTeam() != 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Selected target is not an ally."));
            return false;
        }
    }
    else if (PendingCardTarget == ECardTarget::Tile)
    {
        if (!TraceUnderCursor(BoardTraceChannel, Hit)) return false;
        OutTargetLocation = Hit.ImpactPoint;
    }

    // Comprobación de que no se elige a la misma unidad como objetivo si no es Self
    if (OutTargetUnit == PendingSource && PendingCardTarget != ECardTarget::Ally)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target cannot be the source"));
        return false;
    }

    // Objetivo fuera de rango: se ignora el clic y la carta sigue seleccionada para elegir otro objetivo
    if (!UEgnisCombatLibrary::IsCardTargetInRange(PendingCard, PendingSource, OutTargetUnit, OutTargetLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("BoardPlayerController: objetivo fuera de rango, elige otro"));
        return false;
    }

    return true;
}

void ABoardPlayerController::ExecuteCardPlay(ACharacterBase* TargetUnit, FVector TargetLocation)
{
    if (!PendingCard || !PendingSource || !BM) return;

    const bool bPlayed = BM->PlayCard(PendingCard, PendingSource, TargetUnit, TargetLocation);
    UE_LOG(LogTemp, Warning, TEXT("PlayCard ejecutada"));

    // PlayCard rechazada (sin energía, aturdido...): la carta se queda en la mano
    if (!bPlayed)
    {
        BP_RefreshHandUI();
        PendingCard = nullptr;
        PendingSource = nullptr;
        SelectionState = ECardSelectionState::None;
        CurrentIntent = EInputIntent::Move;
        return;
    }
    
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
                if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>()) 
                    AM->PlayTrigger(EEgnisAudioTrigger::AllySelected, nullptr, SelectedAlly);

                if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, FString::Printf(TEXT("Selected: %s"), *SelectedAlly->GetName()));
                UE_LOG(LogTemp, Log, TEXT("Selected Ally: %s"), *SelectedAlly->GetName());
                return true;
            }
        }
    }
    return false;
}

void ABoardPlayerController::TryMoveSelectedAlly(const FHitResult& Hit)
{
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

            // Intenta mover
            if (BM->RequestMove(SelectedAlly, Tile))
            {
                if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Moved to Tile = (%d, %d)"), Tile.X, Tile.Y));
                
                FClickResult Result;
                Result.bHit = true; 
                Result.bHitBoard = true;
                Result.HitActor = Board; 
                Result.WorldPoint = Hit.ImpactPoint;
                BP_OnclickResolved(Result);
                
                DrawDebugSphere(GetWorld(), Result.WorldPoint, 10.f, 12, FColor::Green, false, 1.0f);
            }
        }
    }
}

// ==========================================
// ======== RESTO DE LAS FUNCIONES ========
// ==========================================

bool ABoardPlayerController::DeprojectCursorToWorld(FVector& OutOrigin, FVector& OutDirection) const
{
    if (FSlateApplication::IsInitialized())
    {
        FVector2D PixelPosition, ViewportPosition;
        USlateBlueprintLibrary::AbsoluteToViewport(this, FSlateApplication::Get().GetCursorPos(), PixelPosition, ViewportPosition);
        if (DeprojectScreenPositionToWorld(static_cast<float>(PixelPosition.X), static_cast<float>(PixelPosition.Y), OutOrigin, OutDirection))
        {
            return true;
        }
    }
    return DeprojectMousePositionToWorld(OutOrigin, OutDirection);
}

bool ABoardPlayerController::TraceUnderCursor(ECollisionChannel Channel, FHitResult& OutHit) const
{
    FVector WorldOrigin, WorldDirection;
    if (!DeprojectCursorToWorld(WorldOrigin, WorldDirection))
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
       CancelPendingCard();
       return;
    }

    AAlly* Caster = GetSelectedCaster(Card);
    if (!Caster)
    {
        return;
    }

    PendingCard = Card;
    PendingSource = Caster;
    PendingCardTarget = Card->GetTarget();
    CurrentIntent = EInputIntent::Action;

    if (PendingCardTarget == ECardTarget::None || PendingCardTarget == ECardTarget::Self)
    {
        ExecuteCardPlay(nullptr, FVector::ZeroVector);
        return;
    }

    SelectionState = ECardSelectionState::SelectingTarget;
    BP_RefreshHandUI();
}

void ABoardPlayerController::CancelPendingCard()
{
    PendingCard = nullptr;
    PendingSource = nullptr;
    SelectionState = ECardSelectionState::None;
    CurrentIntent = EInputIntent::Move;
    BP_RefreshHandUI();
}

void ABoardPlayerController::TryPlayCardAtCursor(UBaseCard* Card)
{
    if (bIsInMenu || !Card) return;

    if (!BM || !BM->IsPlayerTurn())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: Cannot play card: Not player turn"));
        return;
    }

    AAlly* Caster = GetSelectedCaster(Card);
    if (!Caster) return;

    PendingCard = Card;
    PendingSource = Caster;
    PendingCardTarget = Card->GetTarget();

    if (PendingCardTarget == ECardTarget::None || PendingCardTarget == ECardTarget::Self)
    {
        FHitResult Hit;
        if (TraceUnderCursor(BoardTraceChannel, Hit) || TraceUnderCursor(UnitTraceChannel, Hit))
        {
            ExecuteCardPlay(nullptr, FVector::ZeroVector);
            return;
        }
    }
    else
    {
        ACharacterBase* TargetUnit = nullptr;
        FVector TargetLocation = FVector::ZeroVector;
        if (ResolveCardTargetUnderCursor(TargetUnit, TargetLocation))
        {
            ExecuteCardPlay(TargetUnit, TargetLocation);
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("BoardPlayerController: no hay un objetivo valido bajo el raton para %s"), *Card->GetName());
    CancelPendingCard();
}

void ABoardPlayerController::BeginCardDrag(UBaseCard* Card)
{
    if (bIsInMenu || !Card || !BM || !BM->IsPlayerTurn()) return;

    AAlly* Caster = GetSelectedCaster(Card);
    if (!Caster) return;

    PendingCard = Card;
    PendingSource = Caster;
    PendingCardTarget = Card->GetTarget();
    SelectionState = ECardSelectionState::None;
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

    PendingSource->Board->GetTilesInRange(PendingSource->CurrentTile, 
        PendingSource->GetEffectiveRange(PendingCard), OutTiles);
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

//Comprueba que personaje va a usar la carta seleccionada
AAlly* ABoardPlayerController::GetSelectedCaster(const UBaseCard* Card) const
{
    AAlly* Ally = Cast<AAlly>(SelectedAlly);
    if (!IsValid(Ally))
    {
        UE_LOG(LogTemp, Warning, TEXT("BoardPlayerController: selecciona primero el aliado que va a jugar la carta"));
        return nullptr;
    }

    if (!UEgnisCombatLibrary::CanAct(Ally))
    {
        UE_LOG(LogTemp, Warning, TEXT("BoardPlayerController: %s esta aturdido y no puede jugar cartas"), *Ally->GetName());
        return nullptr;
    }

    if (Card && !Card->CanBePlayedBy(Ally))
    {
        UE_LOG(LogTemp, Warning, TEXT("BoardPlayerController: %s no tiene el culto de la carta (%s)"), *Ally->GetName(), *Card->GetCult().ToString());
        return nullptr;
    }

    return Ally;
}
