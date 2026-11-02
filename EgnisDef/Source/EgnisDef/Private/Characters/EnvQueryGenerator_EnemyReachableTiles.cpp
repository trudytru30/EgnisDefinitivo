#include "Characters/EnvQueryGenerator_EnemyReachableTiles.h"
#include "Characters/Enemy.h"
#include "Characters/EnemyCombatLibrary.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Controller.h"

UEnvQueryGenerator_EnemyReachableTiles::UEnvQueryGenerator_EnemyReachableTiles()
{
	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UEnvQueryGenerator_EnemyReachableTiles::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	//El que pregunta puede ser el enemigo o su controlador
	UObject* QueryOwner = QueryInstance.Owner.Get();
	AEnemy* Enemy = Cast<AEnemy>(QueryOwner);
	if (!Enemy)
	{
		if (const AController* Controller = Cast<AController>(QueryOwner))
		{
			Enemy = Cast<AEnemy>(Controller->GetPawn());
		}
	}
	if (!Enemy || !Enemy->Board)
	{
		return;
	}

	TArray<FTileCoord> Tiles = UEnemyCombatLibrary::GetReachableTiles(Enemy);
	if (bIncludeCurrentTile)
	{
		Tiles.Add(Enemy->CurrentTile);
	}

	TArray<FNavLocation> Points;
	Points.Reserve(Tiles.Num());
	for (const FTileCoord& Tile : Tiles)
	{
		Points.Add(FNavLocation(Enemy->GetStandLocationForTile(Tile)));
	}
	QueryInstance.AddItemData<UEnvQueryItemType_Point>(Points);
}

FText UEnvQueryGenerator_EnemyReachableTiles::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Casillas alcanzables (Egnis)"));
}

FText UEnvQueryGenerator_EnemyReachableTiles::GetDescriptionDetails() const
{
	return FText::FromString(bIncludeCurrentTile
		? TEXT("Movimiento del enemigo, incluye su casilla")
		: TEXT("Movimiento del enemigo, sin su casilla"));
}