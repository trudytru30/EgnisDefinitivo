#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_CreateEffect.generated.h"

class AObstacleBase;

// Crea un obstaculo en la casilla elegida (Target Tile). Su vida es la piedad de quien juega la carta
// por PietyMultiplier. La casilla tiene que estar dentro del tablero y libre.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_CreateEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_CreateEffect();

	// Obstaculo que se crea.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CreateEffect")
	TSubclassOf<AObstacleBase> ObstacleClass;

	// Vida del obstaculo = piedad del personaje x este multiplicador.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CreateEffect", meta = (ClampMin = "0"))
	float PietyMultiplier = 1.0f;

	virtual void Execute_Tile(ACharacterBase* Self, FVector Location) override;
	virtual bool CanTargetTile(const ACharacterBase* Self, const FTileCoord& Tile) const override;
};