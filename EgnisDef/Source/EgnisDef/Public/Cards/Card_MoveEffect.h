#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_MoveEffect.generated.h"

// Mueve a quien juega la carta a una casilla elegida, siguiendo el patron de movimiento del personaje
// pero limitado a la distancia de la carta. No gasta el movimiento normal del turno y lo bloquea Paralizado.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_MoveEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_MoveEffect();

	// Falso: la carta mueve Distance casillas. Verdadero: MovementFraction del movimiento del personaje
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MoveEffect")
	bool bUseFractionOfMovement = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MoveEffect", meta = (ClampMin = "0", EditCondition = "!bUseFractionOfMovement"))
	int32 Distance = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MoveEffect", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseFractionOfMovement"))
	float MovementFraction = 0.5f;

	// Casillas que mueve este personaje con esta carta (nunca mas que su movimiento).
	int32 GetDistanceFor(const ACharacterBase* Character) const;

	// True si la casilla es valida para este personaje
	bool CanMoveTo(const ACharacterBase* Character, const FTileCoord& Tile) const;

	virtual void Execute_Tile(ACharacterBase* Self, FVector Location) override;
};