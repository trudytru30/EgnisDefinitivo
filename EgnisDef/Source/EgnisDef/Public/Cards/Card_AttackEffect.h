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
	float DamageMultiplier = 1.0f;

	// GameplayEffect que aplica el daño de verdad (GE_CardDamage).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AttackEffect")
	TSubclassOf<class UGameplayEffect> DamageEffect;
	
	virtual void Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy) override;
	virtual void Execute_Obstacle(ACharacterBase* Self, AObstacleBase* Obstacle) override;
	virtual bool CanTargetObstacle() const override { return true; }
};