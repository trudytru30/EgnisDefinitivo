#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoreAndSystems/Board.h"
#include "GridMovementComponent.generated.h"

// Patrones de movimiento (se mantiene: no depende de Board)
UENUM(BlueprintType)
enum class EMovePattern : uint8
{
	Straight_WBlock,
	Straight_CanPass,
	L_CanStop,
	L_MustEnd,
	Triangle,
	DiagonalPlusRing1,
	StraightLine_Cardinal
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class EGNISDEF_API UGridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridMovementComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	EMovePattern movePattern;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	int32 moveRange = 1;

	// ===== Reconciliado con el Board real (task #4) =====

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Movement")
	ABoard* BoardActor = nullptr;

	UFUNCTION(BlueprintCallable, Category="Movement")
	bool IsTileValid(const FTileCoord& T);

	UFUNCTION(BlueprintCallable, Category="Movement")
	bool getCurrentTile(FTileCoord& OutTile);

	UFUNCTION(BlueprintCallable, Category="Movement")
	void getRachableTiles(TArray<FTileCoord>& OutTiles);

	UFUNCTION(BlueprintCallable, Category="Movement")
	bool canMoveToTile(const FTileCoord& Target);

	// moveToTile queda aqui por si algun Blueprint la invoca directamente, pero
	// el real de movimiento del juego es el de BattleManager.Si algo la llama sin pasar
	// por BattleManager: no respeta turno/bHasMoved.
	UFUNCTION(BlueprintCallable, Category="Movement")
	bool moveToTile(const FTileCoord& Target);

protected:
	virtual void BeginPlay() override;

private:
	void EnsureBoardActor();
	void AddLTiles(const FTileCoord& CurrentTile, TArray<FTileCoord>& OutTiles, bool bCanStopAnywhere);
	void AddTriangleTiles(const FTileCoord& CurrentTile, TArray<FTileCoord>& OutTiles);
	void AddStraightLineCardinalTiles(const FTileCoord& CurrentTile, TArray<FTileCoord>& OutTiles);
	void AddDiagonalPlusRing1Tiles(const FTileCoord& CurrentTile, TArray<FTileCoord>& OutTiles);
};