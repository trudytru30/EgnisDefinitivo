#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_AreaDamageEffect.generated.h"

class UAudioManager;

UCLASS(EditInlineNew)
class EGNISDEF_API UCard_AreaDamageEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AreaDamageEffect")
	float DamageMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AreaDamageEffect")
	int32 AreaRadius = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AreaDamageEffect")
	TSubclassOf<class UGameplayEffect> DamageEffect;

	virtual void Execute_Tile(ACharacterBase* Self, FVector Location) override;
};