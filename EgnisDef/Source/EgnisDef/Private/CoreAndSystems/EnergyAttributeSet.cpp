#include "CoreAndSystems/EnergyAttributeSet.h"

UEnergyAttributeSet::UEnergyAttributeSet()
{
	// Mismos valores por defecto que tenía EnergyComponent.h (MaxPoints=6)
	InitEnergy(6.f);
	InitMaxEnergy(6.f);
}

void UEnergyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetEnergyAttribute())
	{
		// Equivalente a EnergyComponent::ApplyDelta/BeginPlay, que ya hacían este clamp
		// contra MaxPoints
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxEnergy());
	}
}