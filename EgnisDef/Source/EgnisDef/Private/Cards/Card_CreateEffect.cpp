#include "Cards/Card_CreateEffect.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/Board.h"
#include "Environment/ObstacleBase.h"
#include "Engine/World.h"

UCard_CreateEffect::UCard_CreateEffect()
{
	// Necesita que el jugador elija la casilla
	Target = ECardTarget::Tile;
}

void UCard_CreateEffect::Execute_Tile(ACharacterBase* Self, FVector Location)
{
	Super::Execute_Tile(Self, Location);

	// La piedad solo la tienen los aliados
	const AAlly* Ally = Cast<AAlly>(Self);
	if (!Ally || !Self->Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateEffect: quien juega la carta no es un aliado o no tiene tablero."));
		return;
	}

	if (!ObstacleClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateEffect: falta ObstacleClass en la carta, no se crea nada."));
		return;
	}

	ABoard* Board = Self->Board;
	FTileCoord Tile;
	if (!Board->WorldPointToTile(Location, Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateEffect: la posicion esta fuera del tablero."));
		return;
	}

	// IsTileOccupied tambien cuenta como ocupadas las casillas fuera del tablero y las Void
	if (Board->IsTileOccupied(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateEffect: la casilla (%d, %d) no esta libre."), Tile.X, Tile.Y);
		return;
	}

	// Spawn diferido, igual que Board::SpawnActorOnTile: la casilla y la vida se ponen ANTES del BeginPlay,
	// que es donde el obstaculo se registra en el tablero y se centra en su casilla
	const FTransform SpawnTransform(FRotator::ZeroRotator, Board->TileToWorldCenter(Tile));
	AObstacleBase* Obstacle = Self->GetWorld()->SpawnActorDeferred<AObstacleBase>(ObstacleClass, SpawnTransform,
		Board, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Obstacle)
	{
		return;
	}

	// Minimo 1: con piedad 0 el obstaculo nace con 1 de vida. Hace falta porque InitialHealth a 0 significa
	// "usa la vida del Blueprint"
	const float Health = FMath::Max(1.f, Ally->GetCharacterPiety() * PietyMultiplier);
	Obstacle->CurrentTile = Tile;
	Obstacle->bDestructible = true; // si no, ApplyDamage lo ignoraria
	Obstacle->InitialHealth = Health;
	Obstacle->FinishSpawning(SpawnTransform);

	UE_LOG(LogTemp, Log, TEXT("CreateEffect: %s (piedad %.1f x %.1f) crea %s en (%d, %d) con %.1f de vida."),
		*Ally->GetName(), Ally->GetCharacterPiety(), PietyMultiplier, *ObstacleClass->GetName(), Tile.X, Tile.Y,
		Obstacle->HealthComp ? Obstacle->GetMaxHealth() : 0.f);
}

bool UCard_CreateEffect::CanTargetTile(const ACharacterBase* Self, const FTileCoord& Tile) const
{
	if (!Self || !Self->Board)
	{
		return false;
	}

	// IsTileOccupied tambien cuenta como ocupadas las casillas fuera del tablero y las Void
	if (Self->Board->IsTileOccupied(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateEffect: la casilla (%d, %d) no esta libre, elige otra."), Tile.X, Tile.Y);
		return false;
	}
	return true;
}