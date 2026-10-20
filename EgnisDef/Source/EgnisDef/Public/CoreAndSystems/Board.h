
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Board.generated.h"

UENUM(BlueprintType)
enum class ETileType : uint8
{
	Void                  UMETA(DisplayName = "Void"),
	Normal                UMETA(DisplayName = "Normal"),
	Obstacle              UMETA(DisplayName = "Obstacle"),
	DamagingObstacle      UMETA(DisplayName = "DamagingObstacle"),
	BreakableObstacle     UMETA(DisplayName = "BreakableObstacle"),
	ExplosiveObstacle     UMETA(DisplayName = "ExplosiveObstacle"),
	Water                 UMETA(DisplayName = "Water"),
	WaterPlatform         UMETA(DisplayName = "WaterPlatform"),
	Ice                   UMETA(DisplayName = "Ice"),
	Fog                   UMETA(DisplayName = "Fog"),
};

USTRUCT(BlueprintType)
struct FTileCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 X = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Y = -1;

	FTileCoord() = default;
	FTileCoord(int32 InX, int32 InY) : X(InX), Y(InY) {}
};

FORCEINLINE bool operator==(const FTileCoord& A, const FTileCoord& B)
{
	return A.X == B.X && A.Y == B.Y;
}

FORCEINLINE uint32 GetTypeHash(const FTileCoord& T)
{
	return HashCombine(::GetTypeHash(T.X), ::GetTypeHash(T.Y));
}

class ULevelDataAsset;
struct FEnemySpawnInfo;

UCLASS()
class EGNISDEF_API ABoard : public AActor{
	GENERATED_BODY()

public:
	ABoard();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board | Global Visuals")
	TMap<ETileType, TSubclassOf<AActor>> TileVisuals;
	
	// Nivel que se vuelca en la escena al empezar (tamaño, tipos de casilla y enemigos). Sin asset, el tablero
	// se queda con BoardSizeX/BoardSizeY y con lo que se haya colocado a mano en el mapa
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board | Level Data")
	ULevelDataAsset* CurrentLevelData;

	// Si está activo, el tablero carga CurrentLevelData en su BeginPlay. Desactivarlo si otro sistema
	// (p. ej. la run eligiendo el nivel) va a llamar a GenerateBoard él mismo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board | Level Data")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Board)
	float TileSize = 100.f;

	// Con un CurrentLevelData asignado, el tamaño sale de su Width/Height y estos campos quedan en gris
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Board, meta = (EditCondition = "CurrentLevelData == nullptr"))
	int32 BoardSizeX = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Board, meta = (EditCondition = "CurrentLevelData == nullptr"))
	int32 BoardSizeY = 8;

	UFUNCTION(BlueprintCallable, Category ="Board|Grid")
	bool WorldPointToTile(const FVector& WorldPoint, FTileCoord& OutTile) const;

	UFUNCTION(BlueprintCallable, Category="Board|Grid")
	FVector TileToWorldCenter(const FTileCoord& Tile) const;

	UFUNCTION(BlueprintCallable, Category="Board|Occupancy")
	bool IsTileOccupied(const FTileCoord& Tile) const;

	UFUNCTION(BlueprintCallable, Category="Board|Occupancy")
	bool RegisterOccupant(const FTileCoord& Tile, AActor* Occupant);

	UFUNCTION(BlueprintCallable, Category="Board|Occupancy")
	void UnregisterOccupant(const FTileCoord& Tile, AActor* Occupant);

	UFUNCTION(BlueprintCallable, Category="Board|Occupancy")
	void GetNeighbors4_Free(const FTileCoord& From, TArray<FTileCoord>& OutNeighbors) const;

	UFUNCTION(BlueprintCallable, Category="Board|Occupancy")
	AActor* GetTileOccupant(const FTileCoord& Tile) const;

	//Aqui los dos unifican sobre el mismo mapa (OccupiedBy) para que no se desincronicen
	UFUNCTION(BlueprintCallable, Category="Board|Occupancy")
	void SetTileOccupant(const FTileCoord& Tile, AActor* Occupant);

	UFUNCTION(BlueprintCallable, Category="Board|Grid")
	void GetTilesInRange(const FTileCoord& From, int32 Range, TArray<FTileCoord>& OutTiles) const;
	
	bool IsInside(const FTileCoord& Tile) const;

	// Tipo de la casilla según el nivel cargado. Fuera del tablero es Void y sin nivel todo es Normal
	UFUNCTION(BlueprintPure, Category="Board|Grid")
	ETileType GetTileType(const FTileCoord& Tile) const;

	// Centro del tablero en el mundo y su tamaño en unidades. Sirve para ajustar al tablero cosas como el plano de los clics
	UFUNCTION(BlueprintPure, Category="Board|Grid")
	void GetBoardBounds(FVector& OutCenter, FVector2D& OutSize) const;

	// Vuelca CurrentLevelData en la escena: tamaño, tipos de casilla, visuales de casilla y enemigos.
	// Se puede volver a llamar: primero quita lo que generó la vez anterior
	UFUNCTION(BlueprintCallable, Category = "Board|Generation")
	void GenerateBoard();

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:

	UPROPERTY()
	TArray<AActor*> SpawnedTiles;

	// Enemigos que ha creado el tablero desde el nivel (débiles: los que mueren desaparecen solos)
	TArray<TWeakObjectPtr<AActor>> SpawnedEnemies;

	// Copia de los tipos de casilla del nivel, [Y * BoardSizeX + X]. Es una copia porque en partida
	// cambiarán (un obstáculo rompible pasará a Normal) y el asset no se debe tocar
	TArray<ETileType> TileTypes;

	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Board")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Board")
	TObjectPtr<USceneComponent> Corner00;

private:

	// Con nivel asignado, BoardSizeX/BoardSizeY pasan a ser su Width/Height
	void SyncSizeFromLevelData();

	// Quita las casillas y enemigos de la generación anterior (no toca lo colocado a mano en el mapa)
	void ClearGeneratedBoard();

	void SpawnEnemy(const FEnemySpawnInfo& EnemyInfo);

	TMap<FTileCoord, TWeakObjectPtr<AActor>> OccupiedBy;
};

