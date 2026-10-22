#include "Cards/Card_MoveEffect.h"
#include "Characters/CharacterBase.h"
#include "Components/GridMovementComponent.h"
#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "CoreAndSystems/AudioManager.h"

UCard_MoveEffect::UCard_MoveEffect()
{
	// El destino se elige en el tablero: el efecto siempre apunta a una casilla
	Target = ECardTarget::Tile;
}

int32 UCard_MoveEffect::GetDistanceFor(const ACharacterBase* Character) const
{
	if (!Character)
	{
		return 0;
	}

	const UGridMovementComponent* MoveComp = Character->FindComponentByClass<UGridMovementComponent>();
	if (!MoveComp)
	{
		return 0;
	}

	// La resta evita que un error de coma flotante suba una casilla de mas al redondear
	const int32 Wanted = bUseFractionOfMovement
		? FMath::CeilToInt(MoveComp->moveRange * MovementFraction - KINDA_SMALL_NUMBER)
		: Distance;

	return FMath::Clamp(Wanted, 0, MoveComp->moveRange);
}

bool UCard_MoveEffect::CanMoveTo(const ACharacterBase* Character, const FTileCoord& Tile) const
{
	if (!Character || !UEgnisCombatLibrary::CanMove(Character))
	{
		return false;
	}

	const int32 MaxTiles = GetDistanceFor(Character);
	if (MaxTiles <= 0)
	{
		return false;
	}

	UGridMovementComponent* MoveComp = Character->FindComponentByClass<UGridMovementComponent>();
	return MoveComp && MoveComp->canMoveToTileLimited(Tile, MaxTiles);
}

void UCard_MoveEffect::Execute_Tile(ACharacterBase* Self, FVector Location)
{
	Super::Execute_Tile(Self, Location);

	if (!Self || !Self->Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveEffect: Self or Board is null"));
		return;
	}

	FTileCoord Destination;
	if (!Self->Board->WorldPointToTile(Location, Destination))
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveEffect: Location is outside the board"));
		return;
	}

	if (!CanMoveTo(Self, Destination))
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveEffect: %s no puede moverse a (%d,%d) (distancia de la carta: %d)"),
			*Self->GetName(), Destination.X, Destination.Y, GetDistanceFor(Self));
		return;
	}

	if (!Self->SetCurrentTile(Destination))
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveEffect: SetCurrentTile rechazada (%d,%d)"), Destination.X, Destination.Y);
		return;
	}

	Self->SnapToCurrentTile(false);

	if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayLandSound(0);

	UE_LOG(LogTemp, Log, TEXT("MoveEffect: %s -> (%d,%d)"), *Self->GetName(), Destination.X, Destination.Y);
}