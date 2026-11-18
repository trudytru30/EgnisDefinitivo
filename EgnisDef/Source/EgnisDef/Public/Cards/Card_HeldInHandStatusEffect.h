#pragma once

#include "CoreMinimal.h"
#include "Card_StatusEffect.h"
#include "GameplayTagContainer.h"
#include "Card_HeldInHandStatusEffect.generated.h"

// Si la carta sigue en la mano al terminar el turno del jugador, aplica Status a todas las piezas del culto Cult
// que haya en el tablero (sin culto: a todos los aliados). Al jugar la carta no hace nada.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_HeldInHandStatusEffect : public UCard_StatusEffect
{
	GENERATED_BODY()

public:
	UCard_HeldInHandStatusEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "StatusEffect|Hand", meta = (Categories = "Cult"))
	FGameplayTag Cult;

	virtual void Execute_OnTurnEndInHand(UDeckManager* Deck) override;
};
