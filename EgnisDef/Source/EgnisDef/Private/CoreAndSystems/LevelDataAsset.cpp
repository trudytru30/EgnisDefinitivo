#include "CoreAndSystems/LevelDataAsset.h"

ETileType ULevelDataAsset::GetTileType(const FTileCoord& Tile) const
{
	if (GridRows.IsValidIndex(Tile.Y) && GridRows[Tile.Y].Columns.IsValidIndex(Tile.X))
	{
		return GridRows[Tile.Y].Columns[Tile.X];
	}
	return ETileType::Void;
}

bool ULevelDataAsset::IsDeployableTileType(ETileType Type)
{
	switch (Type)
	{
	case ETileType::Normal:
	case ETileType::WaterPlatform:
	case ETileType::Ice:
	case ETileType::Fog:
		return true;
	default:
		return false;
	}
}

bool ULevelDataAsset::IsTileNearEnemy(const FTileCoord& Tile) const
{
	for (const FEnemySpawnInfo& Enemy : EnemiesToSpawn)
	{
		const int32 Distance = FMath::Max(FMath::Abs(Tile.X - Enemy.SpawnCoord.X), FMath::Abs(Tile.Y - Enemy.SpawnCoord.Y));
		if (Distance <= DeployMinDistanceToEnemies)
		{
			return true;
		}
	}
	return false;
}

bool ULevelDataAsset::CanDeployOnTile(const FTileCoord& Tile) const
{
	return IsInsideGrid(Tile) && IsDeployableTileType(GetTileType(Tile)) && !IsTileNearEnemy(Tile);
}

void ULevelDataAsset::ResizeGrid()
{
	GridRows.SetNum(FMath::Max(Height, 1));

	for (FTileRow& Row : GridRows)
	{
		while (Row.Columns.Num() < Width)
		{
			Row.Columns.Add(ETileType::Normal);
		}
		Row.Columns.SetNum(FMath::Max(Width, 1));
	}
}

void ULevelDataAsset::ValidateObjectives()
{
	const auto Outside = [this](const FTileCoord& Tile) { return !IsInsideGrid(Tile); };
	SecondaryObjectives.TilesToProtect.RemoveAll(Outside);
	SecondaryObjectives.TilesToDestroy.RemoveAll(Outside);
	if (SecondaryObjectives.bEscort && (Outside(SecondaryObjectives.EscortStart) || Outside(SecondaryObjectives.EscortEnd)))
	{
		SecondaryObjectives.bEscort = false;
	}
}

void ULevelDataAsset::PostInitProperties()
{
	Super::PostInitProperties();

	// Assets recién creados empiezan con la cuadrícula ya rellena
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		ResizeGrid();
	}
}

void ULevelDataAsset::PostLoad()
{
	Super::PostLoad();
	ResizeGrid();
	ValidateObjectives();
}

#if WITH_EDITOR
void ULevelDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ULevelDataAsset, Width) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULevelDataAsset, Height))
	{
		ResizeGrid();
	}

	// Al encoger el tablero pueden quedar objetivos fuera
	ValidateObjectives();
}
#endif
