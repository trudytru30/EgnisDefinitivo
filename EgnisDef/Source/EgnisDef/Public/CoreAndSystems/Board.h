
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Board.generated.h"

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


UCLASS()
class EGNISDEF_API ABoard : public AActor{
	GENERATED_BODY()

public:
	ABoard();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Board)
	float TileSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Board)
	int32 BoardSizeX = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Board)
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

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Board")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Board")
	TObjectPtr<USceneComponent> Corner00;

private:
	
	TMap<FTileCoord, TWeakObjectPtr<AActor>> OccupiedBy;
};

