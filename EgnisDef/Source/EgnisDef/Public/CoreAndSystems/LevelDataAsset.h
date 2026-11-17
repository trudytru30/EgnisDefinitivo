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

// Las mismas dificultades que Egnis Blockout (Tutorial a Nightmare)
UENUM(BlueprintType)
enum class ELevelDifficulty : uint8
{
	Tutorial,
	Easy,
	Normal,
	Hard,
	VeryHard UMETA(DisplayName = "Very Hard"),
	Nightmare
};

// Objetivos secundarios opcionales del nivel (solo datos: la lógica de juego y las recompensas van aparte)
USTRUCT(BlueprintType)
struct FSecondaryObjectives
{
	GENERATED_BODY()

	// Escolta: llevar un peón de EscortStart (A) a EscortEnd (B)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	bool bEscort = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives", meta = (EditCondition = "bEscort"))
	FTileCoord EscortStart;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives", meta = (EditCondition = "bEscort"))
	FTileCoord EscortEnd;

	// Casillas que el enemigo no debe destruir
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FTileCoord> TilesToProtect;

	// Casillas que el jugador tiene que destruir
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objectives")
	TArray<FTileCoord> TilesToDestroy;

	bool HasAny() const { return bEscort || TilesToProtect.Num() > 0 || TilesToDestroy.Num() > 0; }
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Config")
	ELevelDifficulty Difficulty = ELevelDifficulty::Normal;

	// La matriz visual del diseño del nivel: GridRows[Y].Columns[X]
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Grid")
	TArray<FTileRow> GridRows;

	// Enemigos del nivel
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Enemies")
	TArray<FEnemySpawnInfo> EnemiesToSpawn;

	// No se pueden colocar héroes a esta distancia o menos de un enemigo (cuadrado alrededor, como en Egnis Blockout)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Deploy", meta = (ClampMin = "0"))
	int32 DeployMinDistanceToEnemies = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Objectives")
	FSecondaryObjectives SecondaryObjectives;

#if WITH_EDITORONLY_DATA
	// De qué nivel de Egnis Blockout sale este asset: se usa para reimportar sin duplicar
	UPROPERTY(VisibleAnywhere, Category = "Level|Import")
	FString BlockoutLevelId;

	UPROPERTY(VisibleAnywhere, Category = "Level|Import")
	FString BlockoutLevelName;

	// Último archivo .json importado (vacío si se pegó desde el portapapeles)
	UPROPERTY(VisibleAnywhere, Category = "Level|Import")
	FString BlockoutSourceFile;
#endif

	UFUNCTION(BlueprintPure, Category = "Level|Grid")
	ETileType GetTileType(const FTileCoord& Tile) const;

	// Si en ese tipo de casilla se puede poner un héroe (no es un hueco, un obstáculo ni agua)
	UFUNCTION(BlueprintPure, Category = "Level|Deploy")
	static bool IsDeployableTileType(ETileType Type);

	// Si la casilla está a DeployMinDistanceToEnemies o menos de algún enemigo del nivel (o tiene uno encima)
	UFUNCTION(BlueprintPure, Category = "Level|Deploy")
	bool IsTileNearEnemy(const FTileCoord& Tile) const;

	// Regla del nivel para colocar héroes: dentro del tablero, tipo de casilla válido y lejos de los enemigos.
	// No mira si la casilla está ocupada en la partida (eso lo hace DeployManager con el tablero)
	UFUNCTION(BlueprintPure, Category = "Level|Deploy")
	bool CanDeployOnTile(const FTileCoord& Tile) const;

	// Ajusta GridRows a Height filas x Width columnas. Las casillas nuevas son Normal.
	void ResizeGrid();

	// Quita de los objetivos las casillas que se han quedado fuera del tablero
	void ValidateObjectives();

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	bool IsInsideGrid(const FTileCoord& Tile) const { return Tile.X >= 0 && Tile.X < Width && Tile.Y >= 0 && Tile.Y < Height; }
};
