#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/AudioManager.h"
#include "Characters/Ally.h"
#include "Cards/DeckManager.h"
#include "Cards/BaseCard.h"
#include "Characters/CharacterBase.h"
#include "Characters/Enemy.h"
#include "Components/EnergyComponent.h"
#include "EngineUtils.h"
#include "Components/GridMovementComponent.h"

class AAlly;
// Iniciar combate
void UBattleManager::Initialize(UDeckManager* InDeckManager)
{
	// Recibir el mazo de combate e inicializarlo, despues empieza el combate
	DeckManager = InDeckManager;
	if (DeckManager)
		DeckManager->InitializeDeck();
}

// Inicio del combate, se inicializa el mazo, la vida de los personajes y las posiciones
void UBattleManager::StartBattle()
{
	TurnCount = 0;
	CurrentTurn = ETurnEnum::PlayerTurn;

	// Obtener numero de personajes en el campo
	CharactersOnField.Empty();
	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		CharactersOnField.Add(*It);
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Battle Started! Characters on field: %d"), CharactersOnField.Num());

	StartPlayerTurn();
}

// Turno del player
void UBattleManager::StartPlayerTurn()
{
	if (bBattleIsOver) return;

	// Robar mano al inicio del turno y establecer energia
	for (ACharacterBase* Character : CharactersOnField)
	{
		if (AAlly* Ally = Cast<AAlly>(Character))
			Ally->GainPoints(-1);	// Para reiniciar los puntos al valor default (mirar EnergyComponent)
	}

	// Resetear bHasMoved para todos los aliados al inicio del turno
	for (ACharacterBase* Character : CharactersOnField)
	{
		if (Character && Character->GetTeam() == 0)
		{
			Character->bHasMoved = false;
		}
	}

	if (DeckManager)
	{
		int32 NumCardsToDraw = DeckManager->GetInitialHandSize() - DeckManager->GetHand().Num();
		if (NumCardsToDraw <= 0){
			UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Hand is already full. No cards drawn."));
		}
		else
		{
			DeckManager->DrawCardAmount(NumCardsToDraw);
			UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Drawn %d cards"), DeckManager->GetHand().Num());
		}
	}

	OnPlayerTurnStarted.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Player Turn %d"), TurnCount);
}

// Turno del enemigo
void UBattleManager::StartEnemyTurn()
{
	if (bBattleIsOver) return;

	UE_LOG(LogTemp, Log, TEXT("[BattleManager]:Enemy Turn %d"), TurnCount);

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

	for (AEnemy* Enemy:Enemies)
	{
		Enemy->MoveTowardClosesPlayer();
		Enemy->MakeAction();
	}

	// Comprobar estado de unidades despues de cada turno enemigo y terminar el turno
	UpdateUnitsAlive();
	EndTurn();
}

// Fin del turno y cambio al otro bando
void UBattleManager::EndTurn()
{
	if (bBattleIsOver) return;

	switch ( CurrentTurn)
	{
		case ETurnEnum::PlayerTurn:
			if (DeckManager->GetHand().Num() > 0)
			{
				const TArray<UBaseCard*> HandCopy = DeckManager->GetHand();
				for (UBaseCard* Card : HandCopy)
				{
					DeckManager->DiscardCardFromHand(Card);
				}
			}

			CurrentTurn = ETurnEnum::EnemyTurn;
			StartEnemyTurn();
			break;
		case ETurnEnum::EnemyTurn:
			CurrentTurn = ETurnEnum::PlayerTurn;
			TurnCount++;
			StartPlayerTurn();
			break;
	default: UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Invalid turn"));
	}
}

// Comprobar si el player puede jugar una carta (es decir, que sea su turno y tenga energia)
bool UBattleManager::PlayCard(UBaseCard* Card, AAlly* Character,
	ACharacterBase* TargetCharacter, FVector Location)
{
	if (!Character || !Character->EnergyComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Character or EnergyComp is null"));
		return false;
	}

	// Comprobaciones
	if (CurrentTurn != ETurnEnum::PlayerTurn || !Card || !DeckManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Not player turn or card is null"));
		return false;
	}

	if (Character->EnergyComp->GetCurrentPoints() < Card->GetCost())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Not enough energy to play card"));
		return false;
	}

	// Jugar carta y restar coste
	if (!Character || !Character->EnergyComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: Character or EnergyComp is null"));
		return false;
	}
	Character->LossPoints(Card->GetCost());
	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Played card: %s. Energy left: %d"), *Card->GetName(),
		Character->EnergyComp->GetCurrentPoints());
	Card->Execute(DeckManager, Character, TargetCharacter, Location);
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
			AlliesAlive++;
		}
		else
		{
			EnemiesAlive++;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: Allies alive: %d, Enemies alive: %d"), AlliesAlive, EnemiesAlive);

	if (AlliesAlive == 0)
		EndBattle(false);
	else if (EnemiesAlive == 0)
		EndBattle(true);
}

// Fin del combate
void UBattleManager::EndBattle(bool bPlayerWon)
{
	bBattleIsOver = true;
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

	//TODO: Notificar al GameMode (no entra en prototipo)
}

// Pedir movimiento
bool UBattleManager::RequestMove(ACharacterBase* Unit, const FTileCoord& TargetTile)
{
	if (!Unit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: Unit is null"));
		return false;
	}
	if (!IsPlayerTurn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleManager]: RequestMove failed: Not player turn"));
		return false;
	}
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

	Unit->SnapToCurrentTile(false);
	if (UAudioManager* AM = GetWorld()->GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayLandSound(0);
	Unit->bHasMoved = true;

	UE_LOG(LogTemp, Log, TEXT("[BattleManager]: RequestMove OK: %s -> (%d,%d)"),
		*Unit->GetName(), Unit->CurrentTile.X, Unit->CurrentTile.Y);
	return true;
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