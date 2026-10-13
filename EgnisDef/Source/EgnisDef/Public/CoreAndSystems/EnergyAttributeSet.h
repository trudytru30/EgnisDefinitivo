#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreAndSystems/AttributeSetMacros.h" // Macro ATTRIBUTE_ACCESSORS compartida
#include "EnergyAttributeSet.generated.h"

// Energía: solo la usan las unidades que juegan cartas (AAlly). AEnemy no la necesita,
// actúa mediante MakeAction()/AttackDamage fijo en vez de gastar un recurso de energía.
// Separado de HealthAttributeSet por eso mismo: mantiene el mismo reparto de responsabilidades
// que antes tenía EnergyComponent (solo creado en el constructor de AAlly, nunca en AEnemy).
UCLASS()
class EGNISDEF_API UEnergyAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UEnergyAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Energy")
	FGameplayAttributeData Energy;
	ATTRIBUTE_ACCESSORS(UEnergyAttributeSet, Energy)

	UPROPERTY(BlueprintReadOnly, Category = "Energy")
	FGameplayAttributeData MaxEnergy;
	ATTRIBUTE_ACCESSORS(UEnergyAttributeSet, MaxEnergy)

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};