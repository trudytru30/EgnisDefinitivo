#include "Characters/Ally.h"
#include "CoreAndSystems/AudioManager.h"
#include "Components/EnergyComponent.h"
#include "Components/GridMovementComponent.h"
#include "AbilitySystemComponent.h" // Necesario para ApplyModToAttribute

AAlly::AAlly()
{
	PrimaryActorTick.bCanEverTick = true;
	// CREAR SI O SI AQUI PARA QUE FUNCIONE
	EnergyComp = CreateDefaultSubobject<UEnergyComponent>(TEXT("EnergyComp"));
	MoveComp = CreateDefaultSubobject<UGridMovementComponent>(TEXT("MoveComp"));
	
	// El ASC vive en ACharacterBase (compartido con AEnemy), pero EnergyAttributeSet solo
	// tiene sentido en AAlly, así que se crea aquí en vez de en la clase base.
	EnergyAttributeSet = CreateDefaultSubobject<UEnergyAttributeSet>(TEXT("EnergyAttributeSet"));
}

void AAlly::BeginPlay()
{
	Super::BeginPlay();
}

void AAlly::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAlly::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// Sustituyen a EnergyComp->GetCurrentPoints()/GetMaxPoints() de cara al resto del código y al BP.
// Redondeo con RoundToInt porque FGameplayAttributeData guarda floats, pero el juego trabaja
// en puntos enteros (igual que EnergyComponent usaba int32).
int32 AAlly::GetCurrentEnergy() const
{
	return EnergyAttributeSet ? FMath::RoundToInt(EnergyAttributeSet->GetEnergy()) : 0;
}

int32 AAlly::GetMaxEnergy() const
{
	return EnergyAttributeSet ? FMath::RoundToInt(EnergyAttributeSet->GetMaxEnergy()) : 0;
}

void AAlly::LossPoints(int32 Cost)
{
	// GetAbilitySystemComponent() está heredado de ACharacterBase (IAbilitySystemInterface)
	if (!EnergyAttributeSet || !GetAbilitySystemComponent())
	{
		return;
	}

	// ApplyModToAttribute modifica el atributo directamente sin necesidad de crear un
	// GameplayEffect como asset — pasa por PreAttributeChange (el clamp), así que sigue
	// respetando el límite de 0 a MaxEnergy que ya definía EnergyAttributeSet.
	GetAbilitySystemComponent()->ApplyModToAttribute(
		UEnergyAttributeSet::GetEnergyAttribute(), EGameplayModOp::Additive, -Cost);
}

void AAlly::GainPoints(int32 Bonus)
{
	if (!EnergyAttributeSet || !GetAbilitySystemComponent() || Bonus <= 0)
	{
		return;
	}

	GetAbilitySystemComponent()->ApplyModToAttribute(
		UEnergyAttributeSet::GetEnergyAttribute(), EGameplayModOp::Additive, +Bonus);
}

void AAlly::ResetEnergyForTurn()
{
	if (EnergyComp)
	{
		EnergyComp->ResetPoints();	// En principio no se usa, pero por si acaso
	}

	if (EnergyAttributeSet && GetAbilitySystemComponent())
	{
		// Vuelve a poner Energy al máximo. SetNumericAttributeBase toca el BaseValue directamente,
		// útil para un "reset" instantáneo como este.
		GetAbilitySystemComponent()->SetNumericAttributeBase(
			UEnergyAttributeSet::GetEnergyAttribute(), EnergyAttributeSet->GetMaxEnergy());
	}
}