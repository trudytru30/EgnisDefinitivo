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

// Grupo de casillas donde el jugador puede colocar héroes en la fase de despliegue.
// Es un struct porque son solo datos del nivel: quien decide si se puede desplegar es DeployManager, que las consulta
USTRUCT(BlueprintType)
struct FDeployZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deploy")
	FText Name;

	// Cuántos héroes caben a la vez en esta zona
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deploy", meta = (ClampMin = "1"))
	int32 MaxHeroes = 2;

	// Se pintan desde el editor visual del asset (modo "Zonas de despliegue")
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deploy")
	TArray<FTileCoord> Tiles;
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

	// Únicos sitios donde se pueden colocar héroes. Si no hay ninguna, se puede desplegar en todo el tablero
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Deploy")
	TArray<FDeployZone> DeployZones;

	// Índice de la zona que contiene la casilla, o -1 (INDEX_NONE) si no es de ninguna
	UFUNCTION(BlueprintPure, Category = "Level|Deploy")
	int32 FindDeployZoneIndex(const FTileCoord& Tile) const;

	// Ajusta GridRows a Height filas x Width columnas. Las casillas nuevas son Normal.
	void ResizeGrid();

	// Quita de las zonas las casillas fuera del tablero y las repetidas (una casilla solo puede estar en una zona)
	void ValidateDeployZones();

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};