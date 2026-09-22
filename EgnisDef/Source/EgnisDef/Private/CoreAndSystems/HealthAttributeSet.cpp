#include "CoreAndSystems/HealthAttributeSet.h"

UHealthAttributeSet::UHealthAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
}

void UHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// Siempre se llama a Super primero, la clase base también hace comprobaciones internas
	Super::PreAttributeChange(Attribute, NewValue);

	// GetHealthAttribute() (generado por la macro) da un identificador único de "este es el
	// atributo Health de ESTE AttributeSet". La comparación es contra eso, no contra un string
	// ni un enum, así el compilador avisa si algo está mal escrito.
	if (Attribute == GetHealthAttribute())
	{
		// Equivalente exacto al FMath::Clamp(CurrentHealth+Delta, 0.0f, MaxHealth).
		// Aquí se aplica antes de guardar el valor
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Vacío a propósito: Aquí Health <= 0 y se llamará a algo equivalente
	//  a HandleDeath(). Se deja así ahora para que este paso compile de forma
	// aislada y se pueda probar que el ASC funciona antes de tocar la lógica de muerte.
}