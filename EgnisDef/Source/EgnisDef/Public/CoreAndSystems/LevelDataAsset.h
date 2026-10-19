#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Board.h"
#include "LevelDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FTileRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	TArray<ETileType> Columns;
};

USTRUCT(BlueprintType)
struct FEnemySpawnInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AActor> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FTileCoord SpawnCoord;
};

UCLASS()
class EGNISDEF_API ULevelDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Config", meta = (ClampMin = "1"))
	int32 Width = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Config", meta = (ClampMin = "1"))
	int32 Height = 8;

	// La matriz visual del diseño del nivel: GridRows[Y].Columns[X]
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Grid")
	TArray<FTileRow> GridRows;

	// Enemigos del nivel
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Enemies")
	TArray<FEnemySpawnInfo> EnemiesToSpawn;

	// Ajusta GridRows a Height filas x Width columnas. Las casillas nuevas son Normal.
	void ResizeGrid();

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};