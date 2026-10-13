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

	// GameplayEffect que aplica el daño de verdad (GE_CardDamage). Se asigna por carta,
	// igual que DamageAmount, por si en el futuro alguna carta quiere un GE distinto
	// (por ejemplo, uno que además aplique un tag de "quemado").
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AttackEffect")
	TSubclassOf<class UGameplayEffect> DamageEffect;
	
	virtual void Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy) override;
};