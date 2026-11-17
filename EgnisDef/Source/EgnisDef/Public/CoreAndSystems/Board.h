
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
	Limited               UMETA(DisplayName = "Limited"),
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

// Casilla en llamas (rastro de la Quemadura)
USTRUCT(BlueprintType)
struct FFireTile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Fire")
	FTileCoord Tile;

	// Se apaga al llegar a 0 y baja 1 al empezar cada turno (de cualquier bando).
	// Al crearla vale Turns + 1, porque el turno en el que se crea no cuenta.
	UPROPERTY(BlueprintReadOnly, Category="Fire")
	int32 TurnsLeft = 0;

	// Daño puro al pisarla. Se copia de quien la creo, asi sigue editable por diseño.
	UPROPERTY(BlueprintReadOnly, Category="Fire")
	float Damage = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFireTilesChanged);

class ULevelDataAsset;
class UTileSetDataAsset;
class UInstancedStaticMeshComponent;
struct FEnemySpawnInfo;

UCLASS()
class EGNISDEF_API ABoard : public AActor{
	GENERATED_BODY()

public:
	ABoard();

	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board | Global Visuals")
	TObjectPtr<UTileSetDataAsset> TileSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board | Level Data")
	ULevelDataAsset* CurrentLevelData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board | Level Data")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Board)
	float TileSize = 100.f;
	
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
	
	UFUNCTION(BlueprintCallable, Category="Board|Occupancy")
	void SetTileOccupant(const FTileCoord& Tile, AActor* Occupant);

	UFUNCTION(BlueprintCallable, Category="Board|Grid")
	void GetTilesInRange(const FTileCoord& From, int32 Range, TArray<FTileCoord>& OutTiles) const;
	
	bool IsInside(const FTileCoord& Tile) const;
	
	// ===== Casillas en llamas (rastro de la Quemadura) =====
	// Salta al encender o apagar casillas. La UI y los efectos se refrescan con esto.
	UPROPERTY(BlueprintAssignable, Category="Board|Fire")
	FOnFireTilesChanged OnFireTilesChanged;

	// Enciende la casilla durante Turns turnos (sin contar el actual). Si ya estaba en llamas, se renueva.
	void AddFireTile(const FTileCoord& Tile, int32 Turns, float Damage);

	UFUNCTION(BlueprintPure, Category="Board|Fire")
	bool IsTileOnFire(const FTileCoord& Tile) const;

	// Devuelve false si la casilla no esta en llamas
	bool GetFireTile(const FTileCoord& Tile, FFireTile& OutFire) const;

	UFUNCTION(BlueprintPure, Category="Board|Fire")
	TArray<FFireTile> GetFireTiles() const;

	// Gasta un turno a todas las casillas en llamas y apaga las que llegan a 0.
	// Se llama una vez al empezar cada turno, de cualquier bando.
	void AdvanceFireTiles();

	UFUNCTION(BlueprintPure, Category="Board|Grid")
	ETileType GetTileType(const FTileCoord& Tile) const;

	UFUNCTION(BlueprintPure, Category="Board|Grid")
	void GetBoardBounds(FVector& OutCenter, FVector2D& OutSize) const;


	UFUNCTION(BlueprintCallable, Category = "Board|Generation")
	void GenerateBoard();
	
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedTileActors;
	
	TArray<TWeakObjectPtr<AActor>> SpawnedEnemies;
	
	TArray<ETileType> TileTypes;

	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Board")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Board")
	TObjectPtr<USceneComponent> Corner00;
	
	UPROPERTY(Transient)
	TMap<ETileType, TObjectPtr<UInstancedStaticMeshComponent>> FloorComponents;

private:
	
	void SyncSizeFromLevelData();
	void ClearGeneratedBoard();
	void SpawnActorOnTile(const FTileCoord& Tile, TSubclassOf<AActor> ActorClass);
	void SpawnEnemy(const FEnemySpawnInfo& EnemyInfo);
	
	int32 BuildFloor();

	TMap<FTileCoord, TWeakObjectPtr<AActor>> OccupiedBy;
	TMap<FTileCoord, FFireTile> FireTiles;

	void QueueBoardChanged();
	void BroadcastBoardChanged();
	bool bBoardChangedQueued = false;
};

