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
#include "Abilities/EgnisAbilityLibrary.h"
#include "CoreAndSystems/EgnisGameplayTags.h"

ABoardPlayerController::ABoardPlayerController()
{
    bShowMouseCursor = true;
}

void ABoardPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (IsLocalController()) UpdateHoveredInteractable();
    EnsureAllySelected();
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

    // Started selecciona una sola vez por pulsacion, sin repetir mientras se mantiene la tecla.
    UInputAction* SelectionActions[] = { SelectAlly1Action, SelectAlly2Action, SelectAlly3Action, SelectAlly4Action };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(SelectionActions); ++Index)
    {
        if (SelectionActions[Index])
        {
            EnhancedInput->BindAction(SelectionActions[Index], ETriggerEvent::Started, this,
                &ABoardPlayerController::HandleSelectAllyByIndex, Index);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SelectAlly%dAction not assigned"), Index + 1);
        }
    }
    if (PreviousAllyAction)
    {
        EnhancedInput->BindAction(PreviousAllyAction, ETriggerEvent::Started, this,
            &ABoardPlayerController::SelectAdjacentAlly, -1);
    }
    if (NextAllyAction)
    {
        EnhancedInput->BindAction(NextAllyAction, ETriggerEvent::Started, this,
            &ABoardPlayerController::SelectAdjacentAlly, 1);
    }
    if (!PreviousAllyAction || !NextAllyAction)
    {
        UE_LOG(LogTemp, Warning, TEXT("PreviousAllyAction or NextAllyAction not assigned"));
    }

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
    
    // Mientras se eligen cartas de la mano, los clics en el tablero no hacen nada
    if (DeckManager && DeckManager->IsSelectingFromHand()) return;

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

void ABoardPlayerController::SelectAlly(ACharacterBase* Ally)
{
    SelectedAlly = Ally;
    if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>())
        AM->PlayTrigger(EEgnisAudioTrigger::AllySelected, nullptr, SelectedAlly);

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, FString::Printf(TEXT("Selected: %s"), *SelectedAlly->GetName()));
    UE_LOG(LogTemp, Log, TEXT("Selected Ally: %s"), *SelectedAlly->GetName());
}

void ABoardPlayerController::AutoSelectAlly(ACharacterBase* Ally)
{
    SelectedAlly = Ally;
    BP_OnKeyboardAllySelected(Ally);
}

void ABoardPlayerController::EnsureAllySelected()
{
    if (!BSM || BSM->CurrentPhase != EScenePhase::Combat || !DPM) return;

    if (bCombatSelectionStarted && IsValid(SelectedAlly) && SelectedAlly->GetCurrentHealth() > 0.f) return;

    for (const TPair<TWeakObjectPtr<AAlly>, FName>& Hero : DPM->GetDeployedHeroes())
    {
        AAlly* Ally = Hero.Key.Get();
        if (IsValid(Ally) && Ally->GetCurrentHealth() > 0.f)
        {
            bCombatSelectionStarted = true;
            AutoSelectAlly(Ally);
            return;
        }
    }
}

bool ABoardPlayerController::CanSelectAllyFromInput() const
{
    return IsValid(DPM) && !bIsInMenu && !PendingCard
        && SelectionState == ECardSelectionState::None
        && (!DeckManager || !DeckManager->IsSelectingFromHand());
}

void ABoardPlayerController::HandleSelectAllyByIndex(int32 Index)
{
    SelectAllyByIndex(Index);
}

bool ABoardPlayerController::SelectAllyByIndex(int32 Index)
{
    if (!CanSelectAllyFromInput()) return false;

    const auto& Heroes = DPM->GetDeployedHeroes();
    if (!Heroes.IsValidIndex(Index)) return false;

    AAlly* Ally = Heroes[Index].Key.Get();
    if (!IsValid(Ally) || Ally->GetTeam() != 0 || Ally->GetCurrentHealth() <= 0.f) return false;

    SelectAlly(Ally);
    BP_OnKeyboardAllySelected(Ally);
    return true;
}

void ABoardPlayerController::SelectAdjacentAlly(int32 Direction)
{
    if (!CanSelectAllyFromInput() || (Direction != -1 && Direction != 1)) return;

    const auto& Heroes = DPM->GetDeployedHeroes();
    int32 CurrentIndex = INDEX_NONE;
    if (IsValid(SelectedAlly))
    {
        CurrentIndex = Heroes.IndexOfByPredicate([this](const TPair<TWeakObjectPtr<AAlly>, FName>& Hero)
        {
            return Hero.Key.Get() == SelectedAlly.Get();
        });
    }

    // Sin seleccion en este array, ambas direcciones eligen el primer aliado disponible.
    const int32 Step = CurrentIndex == INDEX_NONE ? 1 : Direction;
    const int32 StartIndex = CurrentIndex == INDEX_NONE ? 0 : CurrentIndex + Step;
    for (int32 Index = StartIndex; Heroes.IsValidIndex(Index); Index += Step)
    {
        // Saltamos bajas sin compactar el array: se conserva la relacion numero/heroe.
        if (SelectAllyByIndex(Index)) return;
    }
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
                SelectAlly(ClickedCharacter);
                return true;
            }
        }
    }
    return false;
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

    if (PauseMenuClass)
    {
        PauseMenuWidget = CreateWidget<UUserWidget>(this, PauseMenuClass);
        if (PauseMenuWidget)
        {
            PauseMenuWidget->AddToViewport(100);
            UGameplayStatics::SetGamePaused(this, true);
        }
    }
}

void ABoardPlayerController::CloseMenu()
{
    if (!bIsInMenu) return;

    UE_LOG(LogTemp, Warning, TEXT("CloseMenu"));
    bIsInMenu = false;

    if (PauseMenuWidget)
    {
        PauseMenuWidget->RemoveFromParent();
        PauseMenuWidget = nullptr;
        UGameplayStatics::SetGamePaused(this, false);
    }

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
    
    // Mientras se eligen cartas de la mano, el clic marca/desmarca en vez de jugar
    if (HandleCardClickWhileSelecting(Card)) return;

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

    const bool bSeeding = (PendingCardTarget == ECardTarget::Ally || PendingCardTarget == ECardTarget::Enemy)
        && UEgnisAbilityLibrary::HasRule(Caster, TAG_Rule_Targeting_Seeding);
    if (PendingCardTarget == ECardTarget::None || PendingCardTarget == ECardTarget::Self || bSeeding)
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

    // Mientras se eligen cartas de la mano no se puede jugar otra carta
    if (DeckManager && DeckManager->IsSelectingFromHand()) return;
    
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

    const bool bSeeding = (PendingCardTarget == ECardTarget::Ally || PendingCardTarget == ECardTarget::Enemy)
        && UEgnisAbilityLibrary::HasRule(Caster, TAG_Rule_Targeting_Seeding);
    if (PendingCardTarget == ECardTarget::None || PendingCardTarget == ECardTarget::Self || bSeeding)
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
    
    // Mientras se eligen cartas de la mano no se puede jugar otra carta
    if (DeckManager && DeckManager->IsSelectingFromHand()) return;

    AAlly* Caster = GetSelectedCaster(Card);
    if (!Caster) return;

    PendingCard = Card;
    PendingSource = Caster;
    PendingCardTarget = Card->GetTarget();
    SelectionState = ECardSelectionState::None;
}

bool ABoardPlayerController::CanUnitMoveThisTurn(ACharacterBase* Unit) const
{
    return BM && BM->CanUnitMove(Unit);
}

bool ABoardPlayerController::RequestUnitMove(ACharacterBase* Unit, const FTileCoord& TargetTile)
{
    return BM && BM->RequestMove(Unit, TargetTile);
}

void ABoardPlayerController::SelectDraggedAlly(ACharacterBase* Unit)
{
    if (IsValid(Unit) && Unit != SelectedAlly && CanSelectAllyFromInput())
    {
        AutoSelectAlly(Unit);
    }
}

void ABoardPlayerController::RequestEndTurn()
{
    if (DeckManager && DeckManager->IsSelectingFromHand())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: Cannot end turn: hay una seleccion de cartas pendiente"));
        return;
    }
    
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

bool ABoardPlayerController::HandleCardClickWhileSelecting(UBaseCard* Card)
{
    if (!DeckManager || !DeckManager->IsSelectingFromHand())
    {
        return false;
    }

    if (Card && !DeckManager->ToggleCardSelection(Card))
    {
        UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: no se puede marcar %s (no es elegible o ya estan marcadas las necesarias)"),
            *Card->GetName());
    }
    return true;
}

void ABoardPlayerController::DebugConfirmHandSelection()
{
    if (!DeckManager || !DeckManager->CanConfirmHandSelection())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: no se puede confirmar: no hay seleccion o faltan cartas por marcar"));
        return;
    }
    DeckManager->ConfirmHandSelection();
}