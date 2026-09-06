#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Characters/CharacterBase.h"
#include "Card_HealEffect.generated.h"

class UAudioManager;


UCLASS(EditInlineNew)
class EGNISDEF_API UCard_HealEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HealEffect")
	float HealAmount = 10.0f;

	virtual void Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally) override;
};
