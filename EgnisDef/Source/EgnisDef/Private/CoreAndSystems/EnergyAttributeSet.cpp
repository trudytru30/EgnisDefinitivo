#include "CoreAndSystems/EnergyAttributeSet.h"

UEnergyAttributeSet::UEnergyAttributeSet()
{
	// Mismos valores por defecto que tenía EnergyComponent.h
	InitEnergy(5.f);
	InitMaxEnergy(5.f);
	InitSecondWindEnergy(0.f);
}

void UEnergyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetEnergyAttribute())
	{
		NewValue = bAllowOverMax ? FMath::Max(NewValue, 0.f) : FMath::Clamp(NewValue, 0.f, GetMaxEnergy());
	}
	else if (Attribute == GetSecondWindEnergyAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}