#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "GameplayTagContainer.h"
#include "Card_AbilityEventEffect.generated.h"

// Manda un evento de gameplay a quien juega la carta para que lo recoja una de sus habilidades
// (por ejemplo, Sed de Sangre consume o llena la furia de Wers). Si no tiene ninguna que lo escuche, no hace nada.
// No sale en la carta.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_AbilityEventEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_AbilityEventEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AbilityEvent", meta = (Categories = "Event"))
	FGameplayTag EventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AbilityEvent")
	float Magnitude = 0.f;

	virtual void Execute_Self(ACharacterBase* Source) override;
};
