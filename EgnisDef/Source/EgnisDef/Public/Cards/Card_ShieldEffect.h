#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Characters/CharacterBase.h"
#include "Card_ShieldEffect.generated.h"

// Da escudo a un aliado (Target Ally, puede ser uno mismo) o a quien juega la carta (Target Self).
// El escudo absorbe daño antes que la vida (ver UHealthAttributeSet::PreGameplayEffectExecute).
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_ShieldEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_ShieldEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ShieldEffect", meta = (ClampMin = "0"))
	float ShieldAmount = 10.0f;

	// GameplayEffect instantaneo (GE_CardShield): Add sobre el atributo Shield con la magnitud
	// Set By Caller de TAG_Data_Damage. Es un duplicado de GE_CardDamage cambiando solo el atributo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ShieldEffect")
	TSubclassOf<class UGameplayEffect> ShieldEffect;

	virtual void Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally) override;
	virtual void Execute_Self(ACharacterBase* Source) override;

private:
	void ApplyShield(ACharacterBase* Source, ACharacterBase* Receiver) const;
};