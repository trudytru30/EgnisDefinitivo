#include "CoreAndSystems/TileSetDataAsset.h"

void UTileSetDataAsset::AddMissingTileTypes()
{
	const UEnum* TileTypeEnum = StaticEnum<ETileType>();

	// NumEnums() - 1: el último valor es el ETileType_MAX que añade el motor
	for (int32 Index = 0; Index < TileTypeEnum->NumEnums() - 1; ++Index)
	{
		const ETileType Type = static_cast<ETileType>(TileTypeEnum->GetValueByIndex(Index));
		if (Type != ETileType::Void)
		{
			TileTypes.FindOrAdd(Type);
		}
	}
}

void UTileSetDataAsset::PostInitProperties()
{
	Super::PostInitProperties();

	// Assets recién creados empiezan con la tabla ya rellena
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AddMissingTileTypes();
	}
}

void UTileSetDataAsset::PostLoad()
{
	Super::PostLoad();
	AddMissingTileTypes();
}
