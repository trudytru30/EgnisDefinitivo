#include "Components/HealthComponent.h"
#include "Characters/CharacterBase.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

}

float UHealthComponent::ApplyDelta(float Delta)
{
	if (Delta == 0.0f)
	{
		return CurrentHealth;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth  = FMath::Clamp(CurrentHealth+Delta,0.0f,MaxHealth);

	// Si el delta es por daño y no por curacion
	const bool bDamageApplied = (Delta < 0.0f) && (CurrentHealth < OldHealth);

	// Si no nos mata aplicamos hitReact
	if (bDamageApplied && CurrentHealth > 0.0f)
	{
		//queu visaul al jugador de daño
	}

	// Si nos mata aplicamos Death
	if (OldHealth > 0.0f &&  CurrentHealth <=0.0f)
	{
		OnDeath();
	}

	// Notificar a la UI que la salud ha cambiado
	if (OnHealthChanged.IsBound())
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	}

	return CurrentHealth;
}

void UHealthComponent::OnDeath()
{
	// PARCHE TEMPORAL  — mientras no exista GAS: antes esto llamaba manualmente a
	// Owner->EndPlay(...), pero CharacterBase::LossHealth() ya comprueba la vida tras aplicar el
	// delta y llama a HandleDeath() -> Destroy() (que dispara EndPlay de verdad, al destruirse el
	// actor). Llamar a EndPlay aqui ademas de eso hacia que se disparara dos veces, la primera
	// mientras el actor seguia "vivo" (sin haberse destruido todavia). Quitamos esa llamada y
	// dejamos que HandleDeath()/Destroy() sea el unico camino que gestiona la muerte de verdad.
	// Cuando se meta GAS, la muerte pasara a gestionarse desde el AttributeSet
	// (PostGameplayEffectExecute) y este parche se sustituira por ese diseño — no es la solucion
	// definitiva, solo evita el bug por ahora.
	bDead = true;
}