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
	float DamageAmount = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AreaDamageEffect")
	int32 AreaRadius = 1;

	virtual void Execute_Tile(ACharacterBase* Self, FVector Location) override;
};