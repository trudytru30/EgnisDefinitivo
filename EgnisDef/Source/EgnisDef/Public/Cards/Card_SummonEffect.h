#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_SummonEffect.generated.h"

class AAlly;
class UHeroDataAsset;

// Invoca una invocacion (espiritu) en la casilla elegida. Su vida es la piedad de quien juega la carta multiplicada
// por PietyMultiplier (minimo 1). La casilla tiene que estar dentro del tablero y libre.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_SummonEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_SummonEffect();

	// Blueprint que se crea: un BP hijo de ASummon con la misma configuracion que BP_Ally
	// (colisiones para que se pueda clicar, etc.).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SummonEffect")
	TSubclassOf<AAlly> SummonClass;

	// DataAsset de la invocacion: malla, patron de movimiento y fila de DT_HeroClasses (rango, piedad,
	// velocidad, poder y cultos). La Vitality de esa fila se ignora, la vida sale de la piedad.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SummonEffect")
	TObjectPtr<UHeroDataAsset> SummonData;

	// Vida de la invocacion = piedad de quien invoca x este multiplicador.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SummonEffect", meta = (ClampMin = "0"))
	float PietyMultiplier = 2.0f;

	// Si esta activo, la invocacion no puede jugar cartas ni moverse el turno en que se invoca.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SummonEffect")
	bool bSummonCooldown = false;

	virtual void Execute_Tile(ACharacterBase* Self, FVector Location) override;
	virtual bool CanTargetTile(const ACharacterBase* Self, const FTileCoord& Tile) const override;
};