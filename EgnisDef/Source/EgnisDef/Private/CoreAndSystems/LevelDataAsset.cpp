#include "CoreAndSystems/LevelDataAsset.h"

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

void ULevelDataAsset::ValidateDeployZones()
{
	TSet<FTileCoord> UsedTiles;
	for (FDeployZone& Zone : DeployZones)
	{
		Zone.Tiles.RemoveAll([this, &UsedTiles](const FTileCoord& Tile)
		{
			const bool bInside = Tile.X >= 0 && Tile.X < Width && Tile.Y >= 0 && Tile.Y < Height;
			bool bAlreadyUsed = false;
			UsedTiles.Add(Tile, &bAlreadyUsed);
			return !bInside || bAlreadyUsed;
		});
	}
}

int32 ULevelDataAsset::FindDeployZoneIndex(const FTileCoord& Tile) const
{
	return DeployZones.IndexOfByPredicate([&Tile](const FDeployZone& Zone)
	{
		return Zone.Tiles.Contains(Tile);
	});
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
	ValidateDeployZones();
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

	// Al encoger el tablero o duplicar una zona pueden quedar casillas inválidas o repetidas
	ValidateDeployZones();
}
#endif
