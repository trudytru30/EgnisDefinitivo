#pragma once

#include "CoreMinimal.h"
#include "Environment/ObstacleBase.h"
#include "ExplosiveObstacle.generated.h"

// Cadaver de ciervo (r1)
// Al quedarse sin vida daña a todo lo que haya en un rombo alrededor
UCLASS()
class EGNISDEF_API AExplosiveObstacle : public AObstacleBase
{
	GENERATED_BODY()

public:
	AExplosiveObstacle();

	// Daño puro (ignora escudo) a cada personaje dentro del radio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle|Explosion", meta=(ClampMin="0"))
	float ExplosionDamage = 10.f;

	// Radio en casillas, en rombo (1 = las 4 de alrededor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle|Explosion", meta=(ClampMin="0"))
	int32 ExplosionRadius = 1;

	// Si tambien daña a otros obstaculos destructibles (puede hacer cadena con otros cadaveres)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle|Explosion")
	bool bDamagesObstacles = true;

	// Para el VFX y el sonido
	// salta justo antes de aplicar el daño
	UFUNCTION(BlueprintImplementableEvent, Category="Obstacle|Explosion")
	void OnExploded(const FTileCoord& Center, int32 Radius);

	// Casillas que alcanza la explosion
	UFUNCTION(BlueprintPure, Category="Obstacle|Explosion")
	TArray<FTileCoord> GetExplosionTiles() const;

	virtual void HandleDestroyed() override;

private:
	bool bExploded = false;
};