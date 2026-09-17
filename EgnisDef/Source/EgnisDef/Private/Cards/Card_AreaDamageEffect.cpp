#include "Cards/Card_AreaDamageEffect.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/AudioManager.h"
#include "Environment/ObstacleBase.h"

void UCard_AreaDamageEffect::Execute_Tile(ACharacterBase* Self, FVector Location)
{
	Super::Execute_Tile(Self, Location);

	if (!Self || !Self->Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("AreaDamageEffect: Self or Board is null"));
		return;
	}

	FTileCoord CenterTile;
	if (!Self->Board->WorldPointToTile(Location, CenterTile))
	{
		UE_LOG(LogTemp, Warning, TEXT("AreaDamageEffect: Location is outside the board"));
		return;
	}

	TArray<FTileCoord> AreaTiles;
	Self->Board->GetTilesInRange(CenterTile, AreaRadius, AreaTiles);
	AreaTiles.Add(CenterTile);

	for (const FTileCoord& Tile : AreaTiles)
	{
		AActor* Occupant = Self->Board->GetTileOccupant(Tile);
		if (!Occupant)
		{
			continue;
		}

		if (ACharacterBase* Character = Cast<ACharacterBase>(Occupant))
		{
			if (Character->GetTeam() != Self->GetTeam())
			{
				Character->LossHealth(DamageAmount);
			}
			continue;
		}

		if (AObstacleBase* Obstacle = Cast<AObstacleBase>(Occupant))
		{
			Obstacle->ApplyDamage(DamageAmount);
		}
	}

	if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayAttackSound(0);
}