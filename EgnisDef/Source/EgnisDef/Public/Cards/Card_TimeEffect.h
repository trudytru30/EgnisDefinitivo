#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_TimeEffect.generated.h"

// Suma o resta tiempo al temporizador de la llama: piedad de quien juega la carta x SecondsPerPiety.
// Con SecondsPerPiety negativo quita tiempo. El temporizador lo recibe a traves de UCombatTimerSubsystem.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_TimeEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_TimeEffect();

	// Multiplicador de puntos de piedad del personaje que juega la carta.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TimeEffect")
	float PietyMultiplier = 1.0f;

	virtual void Execute_Self(ACharacterBase* Source) override;
};