#include "Components/EnergyComponent.h"

UEnergyComponent::UEnergyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnergyComponent::BeginPlay()
{
	Super::BeginPlay();

	
	CurrentPoints = FMath::Clamp(CurrentPoints, 0, MaxPoints); // Clampear valores de los puntos
}

// Aniadir puntos de energia
int32 UEnergyComponent::ApplyDelta(int32 Delta)
{
	CurrentPoints = FMath::Clamp(CurrentPoints + Delta, 0, MaxPoints);
	return CurrentPoints;
}

// Reiniciar energia
int32 UEnergyComponent::ResetPoints()
{
	CurrentPoints = MaxPoints;
	return CurrentPoints;
}