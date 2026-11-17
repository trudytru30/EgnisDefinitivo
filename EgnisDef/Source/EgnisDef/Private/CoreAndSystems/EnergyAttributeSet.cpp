#include "CoreAndSystems/EnergyAttributeSet.h"

UEnergyAttributeSet::UEnergyAttributeSet()
{
	// Mismos valores por defecto que tenía EnergyComponent.h (MaxPoints=6)
	InitEnergy(5.f);
	InitMaxEnergy(5.f);
	InitSecondWindEnergy(0.f);
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
	else if (Attribute == GetSecondWindEnergyAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}