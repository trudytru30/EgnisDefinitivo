#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Components/StatusEffectTypes.h"
#include "Card_StatusEffect.generated.h"

// Aplica un estado alterado al objetivo de la carta durante un numero de turnos del propio objetivo.
// El tipo de objetivo (Enemy, Ally o Self) se elige con el campo Target heredado de UCardEffect.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_StatusEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_StatusEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "StatusEffect")
	EStatusType Status = EStatusType::Status_Exposed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "StatusEffect", meta = (ClampMin = "1"))
	int32 Turns = 1;

	virtual void Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy) override;
	virtual void Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally) override;
	virtual void Execute_Self(ACharacterBase* Source) override;

private:
	void ApplyTo(ACharacterBase* Source, ACharacterBase* Receiver) const;
};