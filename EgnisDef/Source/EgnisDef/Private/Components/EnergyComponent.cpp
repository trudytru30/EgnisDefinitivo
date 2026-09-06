#include "Components/EnergyComponent.h"

UEnergyComponent::UEnergyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnergyComponent::BeginPlay()
{
	Super::BeginPlay();

	// FIX respecto al original: el proyecto viejo llamaba a FMath::Clamp(CurrentPoints, 0.0f, MaxPoints)
	// mezclando un literal float (0.0f) con dos int32 en el mismo template — eso no deduce un tipo
	// consistente y no llega a compilar. Con 0 (int) en vez de 0.0f, los tres argumentos son int32.
	CurrentPoints = FMath::Clamp(CurrentPoints, 0, MaxPoints); // Clampear valores de los puntos
}

// Aniadir puntos de energia
int32 UEnergyComponent::ApplyDelta(int32 Delta)
{
	CurrentPoints += Delta; // Por si a futuro se quiren cartas que sumen puntos de energia
	return CurrentPoints;
}

// Reiniciar energia
int32 UEnergyComponent::ResetPoints()
{
	CurrentPoints = MaxPoints;
	return CurrentPoints;
}