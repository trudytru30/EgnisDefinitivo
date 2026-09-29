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

	// Reutiliza el mismo GE_CardDamage de Card_AttackEffect — Add + Set by
	// Caller funciona igual para curar que para dañar, solo cambia el signo del valor pasado.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HealEffect")
	TSubclassOf<class UGameplayEffect> HealEffect;
	
	virtual void Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally) override;
};
