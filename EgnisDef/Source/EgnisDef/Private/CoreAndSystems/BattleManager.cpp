#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/AudioManager.h"
#include "Characters/Ally.h"
#include "Cards/DeckManager.h"
#include "Cards/BaseCard.h"
#include "Characters/CharacterBase.h"
#include "Characters/Enemy.h"
#include "EngineUtils.h"
#include "Components/GridMovementComponent.h"
#include "Components/StatusEffectComponent.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "CoreAndSystems/Board.h"
#include "Abilities/EgnisAbilityLibrary.h"
#include "CoreAndSystems/CombatCameraSubsystem.h"
#include "CoreAndSystems/CombatTimerSubsystem.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "Engine/World.h"

class AAlly;

namespace
{
	template<typename T>
	void ShuffleInPlace(TArray<T>& Array)
	{
		for (int32 i = Array.Num() - 1; i > 0; --i)
		{
			Array.Swap(i, FMath::RandRange(0, i));
		}
	}

	// Locura: elige al azar un objetivo valido para la carta. false si no hay ninguno
	bool PickMadnessTarget(UBaseCard* Card, AAlly* Caster, const TArray<ACharacterBase*>& OnField, ABoard* Board,
		ACharacterBase*& OutTarget, FVector& OutLocation)
	{
		OutTarget = nullptr;
		OutLocation = FVector::ZeroVector;

		switch (Card->GetTarget())
		{
		case ECardTarget::None:
		case ECardTarget::Self:
			return true; // no hace falta elegir nada

		case ECardTarget::Ally:
		case ECardTarget::Enemy:
		{
			const int32 WantedTeam = Card->GetTarget() == ECardTarget::Ally ? Caster->GetTeam() : 1 - Caster->GetTeam();
			TArray<ACharacterBase*> Valid;
			for (ACharacterBase* Candidate : OnField)
			{
				if (IsValid(Candidate) && Candidate->GetTeam() == WantedTeam && Candidate->GetCurrentHealth() > 0.f
					&& UEgnisCombatLibrary::IsCardTargetInRange(Card, Caster, Candidate, Candidate->GetActorLocation()))
				{
					Valid.Add(Candidate);
				}
			}
			if (Valid.IsEmpty())
			{
				return false;
			}
			OutTarget = Valid[FMath::RandRange(0, Valid.Num() - 1)];
			OutLocation = OutTarget->GetActorLocation();
			return true;
		}

		case ECardTarget::Tile:
		{
			if (!Board)
			{
				return false;
			}
			TArray<FTileCoord> Tiles;
			Board->GetTilesInRange(Caster->CurrentTile, Caster->GetEffectiveRange(Card), Tiles);
			TArray<FVector> Valid;
			for (const FTileCoord& Tile : Tiles)
			{
				if (!Card->CanTargetTile(Caster, Tile))
				{
					continue;
				}
				const FVector Location = Board->TileToWorldCenter(Tile);
				if (UEgnisCombatLibrary::IsCardTargetInRange(Card, Caster, nullptr, Location))
				{
					Valid.Add(Location);
				}
			}
			if (Valid.IsEmpty())
			{
				return false;
			}
			OutLocation = Valid[FMath::RandRange(0, Valid.Num() - 1)];
			return true;
		}
		}
		return false;
	}

	// Locura: si la carta jugada abre una seleccion de la mano (Consumir, Descartar, Guardar), se resuelve sola al azar
	void ResolveMadnessSelection(UDeckManager* Deck)
	{
		for (int32 Guard = 0; Deck->IsSelectingFromHand() && Guard < 20; ++Guard)
		{
			TArray<UBaseCard*> Pool = Deck->GetHand();
			ShuffleInPlace(Pool);
			for (UBaseCard* Candidate : Pool)
			{
				if (Deck->GetSelectedCards().Num() >= Deck->GetRequiredSelection())
				{
					break;
				}
				if (Deck->IsCardSelectable(Candidate) && !Deck->IsCardSelected(Candidate))
				{
					Deck->ToggleCardSelection(Candidate);
				}
			}
			if (!Deck->CanConfirmHandSelection())
			{
				UE_LOG(LogTemp, Warning, TEXT("[Locura] no se pudo resolver sola la seleccion de cartas"));
				return;
			}
			Deck->ConfirmHandSelection();
		}
	}
}

void UBattleManager::SetDeckManager(UDeckManager* InDeckManager)
{
	DeckManager = InDeckManager;
}

// Inicio del combate, se inicializa el mazo, la vida de los personajes y las posiciones
void UBattleManager::StartBattle()
{
	bBattleIsOver = false;
	TurnCount = 0;
	CurrentTurn = ETurnEnum::PlayerTurn;

	CharactersOnField.Empty();
	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		CharactersOnField.Add(*It);
	}
	
	// Tablero (para las casillas en llamas)
	Board = nullptr;
	for (TActorIterator<ABoard> It(GetWorld()); It; ++It)
	{
		Board = *It;
		break;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Battle Started! Characters on field: %d"), CharactersOnField.Num());
	
	// Referencia para la salud del equipo (musica): vida maxima de los aliados al empezar
	InitialTeamMaxHealth = 0.0f;
	for (ACharacterBase* Character : CharactersOnField)
	{
		if (Character && Character->GetTeam() == 0)
		{
			InitialTeamMaxHealth += Character->GetMaxHealth();
		}
	}

	if (UAudioManager* AM = GetWorld()->GetGameInstance()->GetSubsystem<UAudioManager>())
	{
		AM->PlayTrigger(EEgnisAudioTrigger::CombatStart);
		AM->SetTeamHealthRatio(GetTeamHealthRatio());
	}

	if (UCombatTimerSubsystem* Timer = GetCombatTimer())
	{
		Timer->ClearTimerPauses();
		Timer->OnTimeRemainingChanged.AddUniqueDynamic(this, &UBattleManager::HandleCombatTimeRemaining);
	}

	UEgnisAbilityLibrary::SendEventToAllCharacters(this, TAG_Event_Battle_Start);

	// Efectos de camara: reinicia y empieza a escuchar el daño de los aliados que ya estan en el campo
	if (UCombatCameraSubsystem* Cam = GetWorld()->GetSubsystem<UCombatCameraSubsystem>())
	{
		Cam->BeginCombat();
		for (ACharacterBase* Character : CharactersOnField)
		{
			Cam->WatchAlly(Character);
		}
	}
	
	StartPlayerTurn();
}

// Turno del player
void UBattleManager::StartPlayerTurn()
{
	if (bBattleIsOver) return;

	if (UCombatTimerSubsystem* Timer = GetCombatTimer())
	{
		Timer->ResumeTimer(TEXT("EnemyTurn"));
	}
	
	// Gasta un turno a las casillas en llamas
	if (Board) Board->AdvanceFireTiles();
	
	NotifyTeamTurnStart(0);	// Notificar al equipo del jugador
	
	// Robar mano al inicio del turno y establecer energia
	for (ACharacterBase* Character : CharactersOnField)
	{
		if (AAlly* Ally = Cast<AAlly>(Character))
			Ally->ResetEnergyForTurn();	 
	}

	// Resetear bHasMoved para todos los aliados al inicio del turno
	for (ACharacterBase* Character : CharactersOnField)
	{
		if (Character && Character->GetTeam() == 0)
		{
			Character->bHasMoved = false;
			if (AAlly* Ally = Cast<AAlly>(Character))
			{
				Ally->SetSummoningSick(false);
			}
		}
	}

	if (DeckManager)
	{
		int32 NumCardsToDraw = DeckManager->GetInitialHandSize() - DeckManager->GetHand().Num();
		NumCardsToDraw += DeckManager->GetKeptCount();
		
		if (NumCardsToDraw <= 0){
			UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Hand is already full. No cards drawn."));
		}
		else
		{
			DeckManager->DrawCardAmount(NumCardsToDraw);
			UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Drawn %d cards"), DeckManager->GetHand().Num());
		}
		DeckManager->ClearKeptCards();
	}
	
	ResolveMadness();

	OnPlayerTurnStarted.Broadcast();
	
	if (UAudioManager* AM = GetWorld()->GetGameInstance()->GetSubsystem<UAudioManager>())
		AM->PlayTrigger(EEgnisAudioTrigger::PlayerTurnStart);

	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Player Turn %d"), TurnCount);
}

// Turno del enemigo
void UBattleManager::StartEnemyTurn()
{
	if (bBattleIsOver) return;

	if (UCombatTimerSubsystem* Timer = GetCombatTimer())
	{
		Timer->PauseTimer(TEXT("EnemyTurn"));
	}

	// Gasta un turno a las casillas en llamas
	if (Board) Board->AdvanceFireTiles();
	
	NotifyTeamTurnStart(1);
	
	
	if (UAudioManager* AM = GetWorld()->GetGameInstance()->GetSubsystem<UAudioManager>()) 
		AM->PlayTrigger(EEgnisAudioTrigger::EnemyTurnStart);
	
	//identifica enemigos
	TArray<AEnemy*> Enemies;

	for (ACharacterBase* Character : CharactersOnField)
	{
		if (!Character) continue;

		if (Character->GetTeam() == 1)
		{
			if (AEnemy* Enemy = Cast<AEnemy>(Character))
			{
				Enemies.Add(Enemy);
			}
		}
	}

	if (!EnemyTurnDirector)
	{
		EnemyTurnDirector = NewObject<UEnemyTurnDirector>(this);
	}
	EnemyTurnDirector->Initialize(this, EnemyTurnSettings);
	EnemyTurnDirector->StartEnemyPhase(Enemies,
		FOnEnemyPhaseFinished::CreateUObject(this, &UBattleManager::OnEnemyPhaseFinished));
}


void UBattleManager::OnEnemyPhaseFinished()
{
	// Comprobar estado de unidades despues de cada turno enemigo y terminar el turno
	UpdateUnitsAlive();
	EndTurn();
}

// Fin del turno y cambio al otro bando
void UBattleManager::EndTurn()
{
	if (bBattleIsOver) return;
	
	if (bMadnessResolving && CurrentTurn == ETurnEnum::PlayerTurn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: la Locura esta actuando, no se puede terminar el turno todavia"));
		return;
	}

	switch ( CurrentTurn)
	{
		case ETurnEnum::PlayerTurn:
			NotifyTeamTurnEnd(0);
			for (UBaseCard* Card : DeckManager->GetHand())
			{
				if (Card)
				{
					Card->NotifyTurnEndInHand(DeckManager);
				}
			}
			if (DeckManager->GetHand().Num() > 0)
			{
				const TArray<UBaseCard*> HandCopy = DeckManager->GetHand();
				for (UBaseCard* Card : HandCopy)
				{
					if (DeckManager->IsCardKept(Card)) continue;
					DeckManager->DiscardCardFromHand(Card);
				}
			}

			CurrentTurn = ETurnEnum::EnemyTurn;
			StartEnemyTurn();
			break;
		case ETurnEnum::EnemyTurn:
			NotifyTeamTurnEnd(1);
			CurrentTurn = ETurnEnum::PlayerTurn;
			TurnCount++;
			StartPlayerTurn();
			break;
	default: UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Invalid turn"));
	}
}

// Comprobar si el player puede jugar una carta (es decir, que sea su turno y tenga energia)
bool UBattleManager::PlayCard(UBaseCard* Card, AAlly* Character, ACharacterBase* TargetCharacter, FVector Location, bool bFreeCost)
{
	if (bMadnessResolving && !bFreeCost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: la Locura esta actuando, no se puede jugar cartas ahora"));
		return false;
	}
	
	if (!Character || !Character->EnergyAttributeSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Character or EnergyAttributeSet is null"));
		return false;
	}

	if (CurrentTurn != ETurnEnum::PlayerTurn || !Card || !DeckManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Not player turn or card is null"));
		return false;
	}
	
	// Con una seleccion de cartas pendiente no se puede jugar otra carta
	if (DeckManager->IsSelectingFromHand())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: hay una seleccion de cartas pendiente, no se puede jugar %s"), *Card->GetName());
		return false;
	}

	// Aturdido: no puede jugar cartas
	if (!UEgnisCombatLibrary::CanAct(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: %s esta aturdido y no puede jugar cartas"), *Character->GetName());
		return false;
	}
	
	// Asustado: antes de actuar tiene que usar su movimiento para huir
	if (UEgnisCombatLibrary::MustFleeBeforeActing(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: %s esta asustado y tiene que huir antes de jugar cartas"),
			*Character->GetName());
		return false;
	}
	
	// Ira irrefrenable: solo puede jugar cartas de ataque
	if (!UEgnisCombatLibrary::CanPlayCardUnderRage(Card, Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: %s esta en Ira irrefrenable y %s no tiene efecto de ataque, no se puede jugar"),
			*Character->GetName(), *Card->GetName());
		return false;
	}
	
	if (!UEgnisCombatLibrary::IsCardTargetInRange(Card, Character, TargetCharacter, Location))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Target out of range for card %s (caster %s)"),
			*Card->GetName(), *Character->GetName());
		return false;
	}
	
	const int32 Cost = UEgnisCombatLibrary::GetEffectiveCost(Card, Character);
	const bool bNormalCost = Cost == Card->GetCost();
	
	if (!bFreeCost && (bNormalCost ? !Character->CanPayCardCost(Card) : Character->GetCurrentEnergy() < Cost))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Not enough energy to play card"));
		return false;
	}

	if (!Character || !Character->EnergyAttributeSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Character or EnergyAttributeSet is null"));
		return false;
	}
	
	if (!bFreeCost)
	{
		if (bNormalCost)
		{
			Character->PayCardCost(Card);
		}
		else
		{
			Character->LossPoints(Cost);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Played card: %s. Energy left: %d"), *Card->GetName(),
		Character->GetCurrentEnergy());
	Card->Execute(DeckManager, Character, TargetCharacter, Location);

	if (IsValid(Character))
	{
		FGameplayTagContainer CardTags;
		if (Card->GetCult().IsValid())
		{
			CardTags.AddTag(Card->GetCult());
		}
		UEgnisAbilityLibrary::SendEvent(Character, TAG_Event_Card_Played, Character, TargetCharacter, Card->GetCost(), Card, CardTags);
	}
	
	// Sangre hirviente: jugar una carta es una accion y hace daño al lanzador
	if (IsValid(Character) && Character->StatusComp)
	{
		Character->StatusComp->OnOwnerActionTaken(TEXT("carta"));
	}
	
	// Efimero: las efimeras que queden en la mano ya no pueden ser la primera en jugarse
	if (DeckManager && !DeckManager->IsSelectingFromHand())
	{
		DeckManager->ReplaceEphemeralCardsInHand(Card);
	}

	UpdateUnitsAlive();

	return true;
}

// Gestionar las unidades que hay en el campo de batalla (despues de cada carta/ accion enemiga lo suyo es llamar a esto)
void UBattleManager::UpdateUnitsAlive()
{
	int32 AlliesAlive = 0;
	int32 EnemiesAlive = 0;
	
	// Limpiar punteros invalidos
	for (int32 i = CharactersOnField.Num()-1; i >= 0; i--)
	{
		if (!IsValid((CharactersOnField[i])))
			CharactersOnField.RemoveAt(i);
	}

	// Comrpobar cuantos aliados y enemigos quedan en campo
	for (ACharacterBase* Character : CharactersOnField)
	{
		if (Character->GetTeam() == 0)
		{
			const AAlly* Ally = Cast<AAlly>(Character);
			if (!Ally || Ally->CountsForVictory())
			{
				AlliesAlive++;
			}
		}
		else
		{
			EnemiesAlive++;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Allies alive: %d, Enemies alive: %d"), AlliesAlive, EnemiesAlive);
	
	if (UAudioManager* AM = GetWorld()->GetGameInstance()->GetSubsystem<UAudioManager>()) 
		AM->SetTeamHealthRatio(GetTeamHealthRatio());

	if (AlliesAlive == 0)
		EndBattle(false);
	else if (EnemiesAlive == 0)
		EndBattle(true);
}

UCombatTimerSubsystem* UBattleManager::GetCombatTimer() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UCombatTimerSubsystem>() : nullptr;
}

void UBattleManager::HandleCombatTimeRemaining(float TimeRemaining, float TotalTime)
{
	if (TimeRemaining <= 0.f && !bBattleIsOver)
	{
		EndBattle(false);
	}
}

// Fin del combate
void UBattleManager::EndBattle(bool bPlayerWon)
{
	bBattleIsOver = true;
	
	FinishMadness();

	if (UCombatTimerSubsystem* Timer = GetCombatTimer())
	{
		Timer->ClearTimerPauses();
		Timer->OnTimeRemainingChanged.RemoveDynamic(this, &UBattleManager::HandleCombatTimeRemaining);
	}
	
	// Efectos de camara: para el balanceo y deja de escuchar el daño
	if (UCombatCameraSubsystem* Cam = GetWorld()->GetSubsystem<UCombatCameraSubsystem>())
	{
		Cam->EndCombat();
	}

	if (EnemyTurnDirector)
	{
		EnemyTurnDirector->Abort();
	}
	
	if (UAudioManager* AM = GetWorld()->GetGameInstance()->GetSubsystem<UAudioManager>())
		AM->PlayTrigger(bPlayerWon ? EEgnisAudioTrigger::CombatVictory : EEgnisAudioTrigger::CombatDefeat);
	
	if (bPlayerWon)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			TEXT("[BattleManager]: PLAYER WINS"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("[BattleManager]: PLAYER LOST"));
	}

	OnBattleEndedEvent.Broadcast(bPlayerWon);
}

// TODO: estas comprobaciones estaban dentro de RequestMove y se han movido aqui
// para que BoardPlayerController::CanUnitMoveThisTurn las pueda consultar sin mover la unidad
bool UBattleManager::CanUnitMove(ACharacterBase* Unit) const
{
	if (!IsValid(Unit))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: Unit is null"));
		return false;
	}
	if (bBattleIsOver || !IsPlayerTurn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: Not player turn"));
		return false;
	}
	
	if (bMadnessResolving) return false; // la Locura esta actuando
	
	if (Unit->GetTeam() != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: Unit is not ally"));
		return false;
	}
	if (!Unit->Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: Unit has no Board"));
		return false;
	}

	if (Unit->bHasMoved)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: %s already moved this turn"), *Unit->GetName());
		return false;
	}
	
	if (!UEgnisCombatLibrary::CanMove(Unit))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: %s esta paralizado"), *Unit->GetName());
		return false;
	}

	return true;
}

// Pedir movimiento
bool UBattleManager::RequestMove(ACharacterBase* Unit, const FTileCoord& TargetTile)
{
	if (!CanUnitMove(Unit))
	{
		return false;
	}

	UGridMovementComponent* MoveComp = Unit->FindComponentByClass<UGridMovementComponent>();
	if (!MoveComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: %s has no GridMovementComponent"), *Unit->GetName());
		return false;
	}

	if (!MoveComp->canMoveToTile(TargetTile))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: tile (%d,%d) not reachable by MoveComponent"),
			TargetTile.X, TargetTile.Y);
		return false;
	}

	if (!Unit->SetCurrentTile(TargetTile))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: SetCurrentTile rejected (%d,%d)"),
			TargetTile.X, TargetTile.Y);
		return false;
	}

	if (!IsValid(Unit))
	{
		return false;
	}
	
	Unit->bHasMoved = true;

	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: RequestMove OK: %s -> (%d,%d)"),
		*Unit->GetName(), Unit->CurrentTile.X, Unit->CurrentTile.Y);
	
	// Sangre hirviente: moverse es una accion y hace daño
	if (Unit->StatusComp)
	{
		Unit->StatusComp->OnOwnerActionTaken(TEXT("movimiento"));
		UpdateUnitsAlive(); // por si ha muerto
	}
	return true;
}

float UBattleManager::GetTeamHealthRatio() const
{
	if (InitialTeamMaxHealth <= 0.0f)
	{
		return 1.0f;
	}

	// Los aliados muertos ya no estan en el campo, asi que no suman: su vida cuenta como 0 frente a la maxima inicial
	float CurrentHealth = 0.0f;
	for (ACharacterBase* Character : CharactersOnField)
	{
		if (IsValid(Character) && Character->GetTeam() == 0)
		{
			const AAlly* Ally = Cast<AAlly>(Character);
			if (Ally && !Ally->CountsForVictory())
			{
				continue;
			}
			CurrentHealth += FMath::Max(0.0f, Character->GetCurrentHealth());
		}
	}

	return FMath::Clamp(CurrentHealth / InitialTeamMaxHealth, 0.0f, 1.0f);
}

// ===== Getters =====
int32 UBattleManager::GetTurnCount() const
{
	return TurnCount;
}

TArray<ACharacterBase*> UBattleManager::GetCharactersOnField() const
{
	return CharactersOnField;
}

bool UBattleManager::IsPlayerTurn() const
{
	return CurrentTurn == ETurnEnum::PlayerTurn;
}

void UBattleManager::NotifyTeamTurnStart(int32 Team)
{
	// Copia porque un efecto de inicio de turno puede acabar matando a una unidad y alterar CharactersOnField
	const TArray<ACharacterBase*> Snapshot = CharactersOnField;
	for (ACharacterBase* Character : Snapshot)
	{
		if (IsValid(Character) && Character->GetTeam() == Team && Character->StatusComp)
		{
			Character->StatusComp->OnOwnerTurnStart();
		}

		if (IsValid(Character) && Character->GetTeam() == Team)
		{
			UEgnisAbilityLibrary::SendEvent(Character, TAG_Event_Turn_Start, Character, nullptr, TurnCount, nullptr, FGameplayTagContainer());
		}
	}
	
	UpdateUnitsAlive();
}

void UBattleManager::NotifyTeamTurnEnd(int32 Team)
{
	const TArray<ACharacterBase*> Snapshot = CharactersOnField;
	for (ACharacterBase* Character : Snapshot)
	{
		if (IsValid(Character) && Character->GetTeam() == Team && Character->StatusComp)
		{
			Character->StatusComp->OnOwnerTurnEnd();
		}

		if (IsValid(Character) && Character->GetTeam() == Team)
		{
			UEgnisAbilityLibrary::SendEvent(Character, TAG_Event_Turn_End, Character, nullptr, TurnCount, nullptr, FGameplayTagContainer());
		}
	}
}

void UBattleManager::ResolveMadness()
{
	if (!DeckManager || bMadnessResolving)
	{
		return;
	}

	// Aliados locos por orden de aparicion en el campo. Los va resolviendo uno a uno MadnessNextAlly
	MadnessQueue.Reset();
	float StartDelay = 0.f;
	for (ACharacterBase* Character : CharactersOnField)
	{
		AAlly* Ally = Cast<AAlly>(Character);
		if (IsValid(Ally) && Ally->GetTeam() == 0 && Ally->StatusComp
			&& Ally->StatusComp->HasStatus(EStatusType::Status_Madness))
		{
			if (MadnessQueue.IsEmpty())
			{
				StartDelay = Ally->StatusComp->MadnessStartDelay;
			}
			MadnessQueue.Add(Ally);
		}
	}
	if (MadnessQueue.IsEmpty())
	{
		return;
	}

	bMadnessResolving = true;
	OnMadnessStarted.Broadcast();
	ScheduleMadness(&UBattleManager::MadnessNextAlly, StartDelay);
}

// Llama a Func ahora (Delay <= 0) o dentro de Delay segundos
void UBattleManager::ScheduleMadness(void (UBattleManager::*Func)(), float Delay)
{
	UWorld* World = GetWorld();
	if (Delay <= 0.f || !World)
	{
		(this->*Func)();
		return;
	}
	World->GetTimerManager().SetTimer(MadnessTimer, this, Func, Delay, false);
}

// Pasa al siguiente aliado loco de la cola y le arma su plan: un movimiento y N cartas, en orden aleatorio
void UBattleManager::MadnessNextAlly()
{
	if (bBattleIsOver || CurrentTurn != ETurnEnum::PlayerTurn)
	{
		FinishMadness();
		return;
	}

	while (!MadnessQueue.IsEmpty())
	{
		AAlly* Ally = MadnessQueue[0].Get();
		MadnessQueue.RemoveAt(0);
		if (!IsValid(Ally) || !Ally->StatusComp || !Ally->StatusComp->HasStatus(EStatusType::Status_Madness))
		{
			continue; // murio o perdio la Locura antes de que le tocara
		}

		MadnessAlly = Ally;
		MadnessCardsTotal = Ally->StatusComp->MadnessCardsPerTurn;
		MadnessCardsPlayed = 0;

		MadnessPlan.Reset();
		if (UEgnisCombatLibrary::CanMove(Ally) && !Ally->bHasMoved)
		{
			MadnessPlan.Add(EMadnessAction::Move);
		}
		for (int32 i = 0; i < MadnessCardsTotal; ++i)
		{
			MadnessPlan.Add(EMadnessAction::Card);
		}
		ShuffleInPlace(MadnessPlan);

		UE_LOG(LogTemp, Log, TEXT("[Locura] %s empieza su turno loco"), *Ally->GetName());
		MadnessStep();
		return;
	}

	FinishMadness();
}

// Ejecuta la siguiente accion del plan del aliado actual. Las acciones que no se pueden hacer se saltan sin espera
void UBattleManager::MadnessStep()
{
	if (bBattleIsOver || CurrentTurn != ETurnEnum::PlayerTurn)
	{
		FinishMadness();
		return;
	}

	AAlly* Ally = MadnessAlly.Get();
	if (!IsValid(Ally) || !Ally->StatusComp || !Ally->StatusComp->HasStatus(EStatusType::Status_Madness))
	{
		// murio o perdio la Locura (le han hecho daño): termina con el, sigue el siguiente
		MadnessNextAlly();
		return;
	}

	// Se leen antes porque moverse puede matar al aliado (fuego del rastro)
	const float ActionDelay = Ally->StatusComp->MadnessActionDelay;
	const float PreviewDelay = Ally->StatusComp->MadnessCardPreviewDelay;

	while (!MadnessPlan.IsEmpty())
	{
		const EMadnessAction Action = MadnessPlan.Pop();
		if (Action == EMadnessAction::Move)
		{
			if (MadnessDoMove(Ally))
			{
				ScheduleMadness(&UBattleManager::MadnessStep, ActionDelay);
				return;
			}
		}
		else
		{
			if (MadnessPrepareCard(Ally))
			{
				ScheduleMadness(&UBattleManager::MadnessExecuteCard, PreviewDelay);
				return;
			}
			// Sin cartas jugables ahora: los huecos de carta que quedan tampoco tendrian ninguna
			MadnessPlan.RemoveAll([](EMadnessAction A) { return A == EMadnessAction::Card; });
		}
	}

	ScheduleMadness(&UBattleManager::MadnessNextAlly, ActionDelay);
}

// Movimiento aleatorio a una casilla alcanzable. Devuelve true si se movio (o murio al moverse)
bool UBattleManager::MadnessDoMove(AAlly* Ally)
{
	UGridMovementComponent* MoveComp = Ally->FindComponentByClass<UGridMovementComponent>();
	if (!MoveComp)
	{
		return false;
	}

	TArray<FTileCoord> Reachable;
	MoveComp->getRachableTiles(Reachable);
	Reachable.RemoveAll([Ally](const FTileCoord& Tile) { return Tile == Ally->CurrentTile; });
	if (Reachable.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[Locura] %s no tiene casillas a las que moverse"), *Ally->GetName());
		return false;
	}

	const FTileCoord Destination = Reachable[FMath::RandRange(0, Reachable.Num() - 1)];
	const FVector From = Ally->GetActorLocation();
	const FVector To = Ally->GetStandLocationForTile(Destination);
	const FString AllyName = Ally->GetName();

	if (!Ally->SetCurrentTile(Destination))
	{
		return false;
	}
	if (!IsValid(Ally))
	{
		UE_LOG(LogTemp, Log, TEXT("[Locura] %s muere al moverse a (%d,%d)"), *AllyName, Destination.X, Destination.Y);
		return true;
	}

	Ally->bHasMoved = true;
	if (OnMadnessMove.IsBound())
	{
		OnMadnessMove.Broadcast(Ally, From, To); // lo anima el Blueprint
	}
	else
	{
		Ally->SnapToCurrentTile(false); // sin animacion: solo se coloca el actor
	}
	UE_LOG(LogTemp, Log, TEXT("[Locura] %s se mueve a (%d,%d)"), *AllyName, Destination.X, Destination.Y);
	return true;
}

// Elige al azar una carta de la mano que pueda jugarse y su objetivo. Avisa a la UI y la deja pendiente de jugar
bool UBattleManager::MadnessPrepareCard(AAlly* Ally)
{
	MadnessPendingCard.Reset();
	MadnessPendingTarget.Reset();
	if (!DeckManager || DeckManager->IsSelectingFromHand() || !UEgnisCombatLibrary::CanAct(Ally))
	{
		return false;
	}

	TArray<UBaseCard*> Candidates = DeckManager->GetHand();
	ShuffleInPlace(Candidates);
	for (UBaseCard* Card : Candidates)
	{
		if (!IsValid(Card) || !Card->CanBePlayedBy(Ally) || !UEgnisCombatLibrary::CanPlayCardUnderRage(Card, Ally))
		{
			continue;
		}

		ACharacterBase* Target = nullptr;
		FVector Location = FVector::ZeroVector;
		if (!PickMadnessTarget(Card, Ally, CharactersOnField, Board, Target, Location))
		{
			UE_LOG(LogTemp, Log, TEXT("[Locura] %s: %s no tiene objetivo valido, se salta"),
				*Ally->GetName(), *Card->GetName());
			continue;
		}

		MadnessPendingCard = Card;
		MadnessPendingTarget = Target;
		MadnessPendingLocation = Location;
		OnMadnessCardAboutToPlay.Broadcast(Ally, Card, Target, MadnessCardsPlayed + 1, MadnessCardsTotal);
		return true;
	}
	return false;
}

// Juega la carta elegida en MadnessPrepareCard (gratis) y la descarta
void UBattleManager::MadnessExecuteCard()
{
	AAlly* Ally = MadnessAlly.Get();
	UBaseCard* Card = MadnessPendingCard.Get();
	ACharacterBase* Target = MadnessPendingTarget.Get();
	const FVector Location = MadnessPendingLocation;
	MadnessPendingCard.Reset();
	MadnessPendingTarget.Reset();

	if (bBattleIsOver || CurrentTurn != ETurnEnum::PlayerTurn)
	{
		FinishMadness();
		return;
	}
	if (!IsValid(Ally) || !Ally->StatusComp || !Ally->StatusComp->HasStatus(EStatusType::Status_Madness)
		|| !IsValid(Card) || !DeckManager || !DeckManager->GetHand().Contains(Card))
	{
		MadnessStep(); // algo cambio durante la espera: se reevalua
		return;
	}

	const float ActionDelay = Ally->StatusComp->MadnessActionDelay;
	const FString AllyName = Ally->GetName();
	const FString CardName = Card->GetName();
	const FString TargetName = Target ? Target->GetName() : FString(TEXT("casilla o sin objetivo"));

	if (!PlayCard(Card, Ally, Target, Location, true))
	{
		MadnessStep(); // rechazada (aturdido, Ira irrefrenable...): la carta se queda en la mano
		return;
	}
	if (bBattleIsOver)
	{
		return;
	}

	DeckManager->DiscardCardFromHand(Card);
	++MadnessCardsPlayed;
	UE_LOG(LogTemp, Log, TEXT("[Locura] %s juega %s sobre %s (%d/%d)"),
		*AllyName, *CardName, *TargetName, MadnessCardsPlayed, MadnessCardsTotal);

	ResolveMadnessSelection(DeckManager);
	ScheduleMadness(&UBattleManager::MadnessStep, ActionDelay);
}

// Fin de la secuencia (o la corta si acaba el combate / el turno)
void UBattleManager::FinishMadness()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MadnessTimer);
	}
	MadnessQueue.Reset();
	MadnessPlan.Reset();
	MadnessAlly.Reset();
	MadnessPendingCard.Reset();
	MadnessPendingTarget.Reset();

	if (bMadnessResolving)
	{
		bMadnessResolving = false;
		OnMadnessFinished.Broadcast();
	}
}

void UBattleManager::RegisterUnit(ACharacterBase* Unit)
{
	if (IsValid(Unit))
	{
		CharactersOnField.AddUnique(Unit);

		// Efectos de camara: las unidades nuevas (invocaciones) tambien sacuden la camara al recibir daño
		if (UCombatCameraSubsystem* Cam = GetWorld()->GetSubsystem<UCombatCameraSubsystem>())
		{
			Cam->WatchAlly(Unit);
		}
	}
}