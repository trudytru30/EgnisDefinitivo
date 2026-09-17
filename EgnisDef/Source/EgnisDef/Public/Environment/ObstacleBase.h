#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoreAndSystems/Board.h"
#include "ObstacleBase.generated.h"


// De momento esta clase cubre solo los dos obstaculos que necesitamos ya: uno que
// simplemente bloquea el paso (BP_Obstacle_Rock, bDestructible=false) y uno destructible
//Los demas obstaculos no estan cubiertos y habria que ampliar codigo
//En ese caso se haria un bP_ObstacleBase padre como ally
//Rock y tronco van con false y true en destructible(prueba:tronco tiene vida a 20)
class UHealthComponent;

UCLASS()
class EGNISDEF_API AObstacleBase : public AActor
{
	GENERATED_BODY()

public:
	AObstacleBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	UHealthComponent* HealthComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	FTileCoord CurrentTile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Obstacle")
	bool bDestructible = false;

	UFUNCTION(BlueprintCallable, Category="Obstacle")
	void ApplyDamage(float Amount);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Grid")
	TObjectPtr<ABoard> Board = nullptr;

private:
	void HandleDestroyed();
	bool bDestroyHandled = false;
};