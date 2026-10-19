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
}
#endif
