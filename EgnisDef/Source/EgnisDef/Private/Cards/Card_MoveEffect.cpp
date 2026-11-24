#include "Cards/Card_MoveEffect.h"
#include "Cards/BaseCard.h"
#include "Characters/Ally.h"
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

bool UCard_MoveEffect::CanTargetTile(const ACharacterBase* Self, const FTileCoord& Tile) const
{
	return CanMoveTo(Self, Tile);
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

	if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) 
		AM->PlayTrigger(EEgnisAudioTrigger::CardMove, SoundOverride, Self);

	UE_LOG(LogTemp, Log, TEXT("MoveEffect: %s -> (%d,%d)"), *Self->GetName(), Destination.X, Destination.Y);
}

bool UCard_MoveEffect::FindTileBeforeTarget(const ACharacterBase* Self, const ACharacterBase* Victim, FTileCoord& OutTile) const
{
	if (!IsValid(Self) || !IsValid(Victim) || !Self->Board)
	{
		return false;
	}

	const FTileCoord From = Self->CurrentTile;
	const FTileCoord To = Victim->CurrentTile;
	if (FMath::Abs(From.X - To.X) + FMath::Abs(From.Y - To.Y) <= 1)
	{
		OutTile = From;
		return true;
	}

	bool bFound = false;
	int32 BestMoved = MAX_int32;
	const FTileCoord Neighbors[] = { FTileCoord(To.X + 1, To.Y), FTileCoord(To.X - 1, To.Y), FTileCoord(To.X, To.Y + 1), FTileCoord(To.X, To.Y - 1) };
	for (const FTileCoord& Tile : Neighbors)
	{
		const int32 Moved = FMath::Abs(From.X - Tile.X) + FMath::Abs(From.Y - Tile.Y);
		if (Moved < BestMoved && CanMoveTo(Self, Tile))
		{
			BestMoved = Moved;
			OutTile = Tile;
			bFound = true;
		}
	}
	if (!bFound)
	{
		const UBaseCard* Card = GetTypedOuter<UBaseCard>();
		const AAlly* Ally = Cast<AAlly>(Self);
		const int32 Reach = (Card ? Card->GetRange() : 0) + (Ally ? Ally->GetCharacterRange() : 1);
		if (FMath::Abs(From.X - To.X) + FMath::Abs(From.Y - To.Y) <= Reach)
		{
			OutTile = From;
			bFound = true;
		}
	}
	return bFound;
}

bool UCard_MoveEffect::CanTargetCharacter(const ACharacterBase* Self, const ACharacterBase* Victim) const
{
	if (Target != ECardTarget::Enemy)
	{
		return Super::CanTargetCharacter(Self, Victim);
	}
	FTileCoord Tile;
	return FindTileBeforeTarget(Self, Victim, Tile);
}

int32 UCard_MoveEffect::GetExtraRange(const ACharacterBase* Self) const
{
	return Target == ECardTarget::Enemy && UEgnisCombatLibrary::CanMove(Self) ? GetDistanceFor(Self) : 0;
}

void UCard_MoveEffect::Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy)
{
	Super::Execute_Enemy(Self, Enemy);

	FTileCoord Destination;
	if (!FindTileBeforeTarget(Self, Enemy, Destination) || Destination == Self->CurrentTile)
	{
		return;
	}
	Execute_Tile(Self, Self->Board->TileToWorldCenter(Destination));
}
