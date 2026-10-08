#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreAndSystems/AttributeSetMacros.h" // Macro ATTRIBUTE_ACCESSORS compartida
#include "HealthAttributeSet.generated.h"

// Vida: presente en todos los personajes (ACharacterBase), tanto AAlly como AEnemy.
// Sustituye a HealthComponent, que ya vivía en ACharacterBase para ambos por igual.
UCLASS()
class EGNISDEF_API UHealthAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UHealthAttributeSet();

	// FGameplayAttributeData no es un float normal: por dentro guarda un "BaseValue" (el valor
	// permanente) y un "CurrentValue" (el valor tras aplicar buffs/debuffs temporales). Esto es
	// justo lo que HealthComponent no tenía, ahí solo había un float, así que cualquier
	// modificador temporal había que gestionarlo a mano.
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData Health;
	// Genera GetHealth(), GetHealthAttribute(), SetHealth(), InitHealth() automáticamente
	ATTRIBUTE_ACCESSORS(UHealthAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UHealthAttributeSet, MaxHealth)

	// Escudo: absorbe el daño antes que Health. Vive en este AttributeSet porque
	// PreGameplayEffectExecute solo se llama en el dueño del atributo que se va a modificar,
	// y la absorcion hay que hacerla justo cuando un GE va a restar Health.
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UHealthAttributeSet, Shield)
	
	// Se llama justo ANTES de aplicar un GE instantaneo. Aqui el escudo se come parte del daño.
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	
	// Se llama automáticamente después de que un GameplayEffect haya modificado un atributo.
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	// Se llama antes de que el cambio se aplique, aquí se hace el clamp (evitar que Health se
	// pase de MaxHealth o baje de 0). Equivalente a los FMath::Clamp que ya había en
	// HealthComponent::ApplyDelta.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};