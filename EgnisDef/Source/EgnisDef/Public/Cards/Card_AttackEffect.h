#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Characters/CharacterBase.h"
#include "Card_AttackEffect.generated.h"

class UAudioManager;


UCLASS(EditInlineNew)
class EGNISDEF_API UCard_AttackEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AttackEffect")
	float DamageAmount = 10.0f;

	virtual void Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy) override;
};