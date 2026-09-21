#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h" // Trae FGameplayEffectModCallbackData y las macros GAMEPLAYATTRIBUTE_*
#include "AttributeSet.h"           // Clase base UAttributeSet de la que hereda
#include "CharacterAttributeSet.generated.h"

// GAS espera que cada atributo tenga 4 funciones: un getter que devuelve la propiedad entera
// (FGameplayAttribute), un getter del valor float, un setter, y un "initter" para poner el valor
// inicial sin pasar por la maquinaria de GameplayEffects. Escribir esas 4 funciones a mano por
// cada atributo es un coñazo y siempre igual, así que Epic da estas macros para generarlas.
// Esto NO es específico de vuestro proyecto, es el boilerplate estándar de cualquier AttributeSet.
// TODO: Los comentarios los ha hecho claude y me hace gracia que diga coñazo asi que se queda asi
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class EGNISDEF_API UCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UCharacterAttributeSet();

	// FGameplayAttributeData no es un float normal: por dentro guarda un "BaseValue" (el valor
	// permanente) y un "CurrentValue" (el valor tras aplicar buffs/debuffs temporales). Esto es
	// justo lo que HealthComponent/EnergyComponent NO tenían, ahí solo había un float, así que
	// cualquier modificador temporal había que gestionarlo a mano.
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData Health;
	// Genera GetHealth(), GetHealthAttribute(), SetHealth(), InitHealth() automáticamente
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, MaxHealth)

	// Energy sustituye a EnergyComponent::CurrentPoints
	UPROPERTY(BlueprintReadOnly, Category = "Energy")
	FGameplayAttributeData Energy;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, Energy)

	// MaxEnergy sustituye a EnergyComponent::MaxPoints
	UPROPERTY(BlueprintReadOnly, Category = "Energy")
	FGameplayAttributeData MaxEnergy;
	ATTRIBUTE_ACCESSORS(UCharacterAttributeSet, MaxEnergy)

	// Se llama automáticamente DESPUÉS de que un GameplayEffect haya modificado un atributo.
	// Aquí es donde en la Fase 4 meteremos "si Health llega a 0, dispara la muerte" — es el
	// equivalente GAS a lo que hoy hace HandleDeath(), pero centralizado en vez de disperso.
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	// Se llama ANTES de que el cambio se aplique, aquí es donde se hace el clamp (evitar que
	// Health se pase de MaxHealth o baje de 0). Es el equivalente a los FMath::Clamp que hay
	// en HealthComponent::ApplyDelta y EnergyComponent::ApplyDelta/BeginPlay.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};