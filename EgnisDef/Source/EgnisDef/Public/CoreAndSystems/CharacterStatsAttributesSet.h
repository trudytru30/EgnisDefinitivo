#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreAndSystems/AttributeSetMacros.h" // Macro ATTRIBUTE_ACCESSORS compartida
#include "CharacterStatsAttributesSet.generated.h"

// Estadisticas de los heroes que vienen de DT_HeroClasses (FHeroClassData): Power, Piety y Speed.
// Solo las usan los aliados (AAlly), igual que EnergyAttributeSet. El valor base lo pone
// AAlly::InitializeFromData desde la tabla; las heridas, pasivas y reliquias podran modificarlas con GameplayEffects.
UCLASS()
class EGNISDEF_API UCharacterStatsAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	// Multiplicador de cartas/efectos
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData Power;
	ATTRIBUTE_ACCESSORS(UCharacterStatsAttributeSet, Power)

	// Escala los efectos de piedad
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData Piety;
	ATTRIBUTE_ACCESSORS(UCharacterStatsAttributeSet, Piety)

	// Casillas de movimiento
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData Speed;
	ATTRIBUTE_ACCESSORS(UCharacterStatsAttributeSet, Speed)
	
	// Rango del personaje: se suma al rango de la carta
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData Range;
	ATTRIBUTE_ACCESSORS(UCharacterStatsAttributeSet, Range)

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};