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

	// Turnos del receptor que dura el escudo: caduca al empezar su turno numero ShieldTurns despues de recibirlo.
	// Con 1 (por defecto) se va al empezar su siguiente turno. 0 = permanente, no caduca.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ShieldEffect", meta = (ClampMin = "0"))
	int32 ShieldTurns = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ShieldEffect", meta = (ClampMin = "0"))
	float ShieldAmount = 10.0f;

	// GameplayEffect instantaneo (GE_CardShield): Add sobre el atributo Shield con la magnitud
	// Set By Caller de TAG_Data_Damage. Es un duplicado de GE_CardDamage cambiando solo el atributo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ShieldEffect")
	TSubclassOf<class UGameplayEffect> ShieldEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ShieldEffect", meta = (ClampMin = "0"))
	float PietyMultiplier = 1.0f;

	UFUNCTION(BlueprintPure, Category = "ShieldEffect")
	float GetFinalAmount(const ACharacterBase* SelfChar) const;
	
	virtual void Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally) override;
	virtual void Execute_Self(ACharacterBase* Source) override;

private:
	void ApplyShield(ACharacterBase* Source, ACharacterBase* Receiver) const;
};