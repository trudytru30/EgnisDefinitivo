#include "CoreAndSystems/CharacterAttributeSet.h"

UCharacterAttributeSet::UCharacterAttributeSet()
{
	// Mismos valores por defecto que ya había en HealthComponent.h (MaxHealth=100)
	// y EnergyComponent.h (MaxPoints=6) — no cambia el balance del juego, solo dónde vive el dato
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitEnergy(6.f);
	InitMaxEnergy(6.f);
}

void UCharacterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// Siempre llamar a Super primero, la clase base hace comprobaciones internas
	Super::PreAttributeChange(Attribute, NewValue);

	// GetHealthAttribute() (generado por la macro) nos da un identificador único de "este es
	// el atributo Health de ESTE AttributeSet". Comparamos contra eso, no contra un string ni
	// un enum, porque así el compilador nos avisa si algo está mal escrito.
	if (Attribute == GetHealthAttribute())
	{
		// Equivalente exacto al FMath::Clamp(CurrentHealth+Delta, 0.0f, MaxHealth) que había en
		// HealthComponent::ApplyDelta, solo que aquí se aplica ANTES de guardar el valor, no después
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetEnergyAttribute())
	{
		// Igual que EnergyComponent::ApplyDelta hacía con CurrentPoints
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxEnergy());
	}
}

void UCharacterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// De momento vacío a propósito: en la Fase 4 se detectará aquí Health <= 0 y se llamará a
	// algo equivalente a HandleDeath(). Se deja así ahora para que este paso compile solo
	// y se pueda probar que el ASC funciona antes de tocar la lógica de muerte.
}