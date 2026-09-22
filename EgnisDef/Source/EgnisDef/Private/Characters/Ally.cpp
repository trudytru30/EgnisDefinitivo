#include "Characters/Ally.h"
#include "CoreAndSystems/AudioManager.h"
#include "Components/EnergyComponent.h"
#include "Components/GridMovementComponent.h"

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

void AAlly::LossPoints(int32 Cost)
{
	if (!EnergyComp)
	{
		return;
	}

	EnergyComp->ApplyDelta(-Cost);
}

void AAlly::GainPoints(int32 Bonus)
{
	if (!EnergyComp || Bonus <= 0)
	{
		return;
	}
	EnergyComp->ApplyDelta(+Bonus);
}

void AAlly::ResetEnergyForTurn()
{
	if (EnergyComp)
	{
		EnergyComp->ResetPoints();
	}
}
