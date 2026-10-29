#include "Characters/Ally.h"
#include "Characters/HeroDataAsset.h"
#include "CoreAndSystems/AudioManager.h"
#include "Components/GridMovementComponent.h"
#include "AbilitySystemComponent.h" // Necesario para ApplyModToAttribute
#include "Cards/BaseCard.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreAndSystems/CharacterStatsAttributesSet.h"

AAlly::AAlly()
{
	PrimaryActorTick.bCanEverTick = true;
	MoveComp = CreateDefaultSubobject<UGridMovementComponent>(TEXT("MoveComp"));

	ModelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ModelMesh"));
	ModelMesh->SetupAttachment(GetCapsuleComponent());
	ModelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// El ASC vive en ACharacterBase (compartido con AEnemy), pero EnergyAttributeSet solo
	// tiene sentido en AAlly, así que se crea aquí en vez de en la clase base.
	EnergyAttributeSet = CreateDefaultSubobject<UEnergyAttributeSet>(TEXT("EnergyAttributeSet"));
	StatsAttributeSet = CreateDefaultSubobject<UCharacterStatsAttributeSet>(TEXT("StatsAttributeSet"));
}

void AAlly::BeginPlay()
{
	Super::BeginPlay();

	// Cada vez que cambie Speed (tabla, herida, bufo...) se actualiza el movimiento
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UCharacterStatsAttributeSet::GetSpeedAttribute())
			.AddUObject(this, &AAlly::OnSpeedChanged);
	}
}

void AAlly::OnSpeedChanged(const FOnAttributeChangeData& Data)
{
	RefreshMoveRange();
}

void AAlly::RefreshMoveRange()
{
	if (!MoveComp)
	{
		return;
	}

	MoveComp->moveRange = GetCharacterSpeed();
	if (MoveComp->moveRange <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Aliado %s: Speed 0, no podra moverse."), *GetName());
	}
}

void AAlly::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAlly::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AAlly::InitializeFromData(UHeroDataAsset* HeroData)
{
    if (!HeroData) return;

    // 1. Apariencia y Animación (Asumiendo que CharacterMesh y AnimBP están en el DataAsset)
    if (HeroData->CharacterModel && ModelMesh)
    {
        ModelMesh->SetStaticMesh(HeroData->CharacterModel);
        ModelMesh->SetRelativeLocation(FVector(0.f, 0.f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
        ModelMesh->SetRelativeScale3D(FVector(HeroData->ModelScale));
        if (HeroData->CharacterMaterial)
        {
            ModelMesh->SetMaterial(0, HeroData->CharacterMaterial);
        }

        TInlineComponentArray<UStaticMeshComponent*> StaticMeshes(this);
        for (UStaticMeshComponent* Component : StaticMeshes)
        {
            if (Component != ModelMesh)
            {
                Component->SetVisibility(false);
            }
        }
    }

    // 3. Inicializar Estadísticas usando GAS
    if (GetAbilitySystemComponent())
    {
    }
	
	// Estadísticas del personaje desde la tabla de stats (columna Range y Piety de DT_HeroClasses)
	if (const FHeroClassData* Stats = HeroData->HeroStatsRow.GetRow<FHeroClassData>(TEXT("AAlly::InitializeFromData")))
	{
		// Power, Piety y Speed viven en GAS: la tabla da el valor base
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			ASC->SetNumericAttributeBase(UCharacterStatsAttributeSet::GetPowerAttribute(), Stats->Power);
			ASC->SetNumericAttributeBase(UCharacterStatsAttributeSet::GetPietyAttribute(), Stats->Piety);
			ASC->SetNumericAttributeBase(UCharacterStatsAttributeSet::GetSpeedAttribute(), Stats->Speed);
			ASC->SetNumericAttributeBase(UCharacterStatsAttributeSet::GetRangeAttribute(), Stats->Range);
		}
		UE_LOG(LogTemp, Log, TEXT("Aliado %s: Power %.1f, Piety %.1f, Speed %d"),
			*GetName(), GetCharacterPower(), GetCharacterPiety(), GetCharacterSpeed());
		
		RefreshMoveRange();
		
		// Colores de carta del personaje: uno por cada culto de su clase. Un culto sin color asignado se ignora
		Cults = Stats->AllowedCults;
		UE_LOG(LogTemp, Log, TEXT("Aliado %s: cultos %s"), *GetName(), *Cults.ToStringSimple());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Aliado %s: sin fila de stats en el DataAsset,"
								" las stats se quedan a 0."), *GetName());
	}
	
    UE_LOG(LogTemp, Log, TEXT("Aliado %s inicializado desde DataAsset."), *GetName());
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
	if (EnergyAttributeSet && GetAbilitySystemComponent())
	{
		// Vuelve a poner Energy al máximo. SetNumericAttributeBase toca el BaseValue directamente,
		// útil para un "reset" instantáneo como este.
		GetAbilitySystemComponent()->SetNumericAttributeBase(
			UEnergyAttributeSet::GetEnergyAttribute(), EnergyAttributeSet->GetMaxEnergy());
	}
}

int32 AAlly::GetCharacterRange() const
{
	
	return StatsAttributeSet ? FMath::Max(0, FMath::RoundToInt(StatsAttributeSet->GetRange())) : 0;
}

int32 AAlly::GetEffectiveRange(const UBaseCard* Card) const
{
	if (!Card)
	{
		return 0;
	}
	return Card->GetRange() + GetCharacterRange();
}

bool AAlly::HasArchetypeColor(EColorType Color) const
{
	return HasCult(EgnisCults::FromLegacyColor(Color));
}

int32 AAlly::GetCurrentEnergy() const
{
	return EnergyAttributeSet ? FMath::RoundToInt(EnergyAttributeSet->GetEnergy()) : 0;
}

int32 AAlly::GetMaxEnergy() const
{
	return EnergyAttributeSet ? FMath::RoundToInt(EnergyAttributeSet->GetMaxEnergy()) : 0;
}

float AAlly::GetCharacterPiety() const
{
	return StatsAttributeSet ? FMath::Max(0.f, StatsAttributeSet->GetPiety()) : 0.f;
}

float AAlly::GetCharacterPower() const
{
	return StatsAttributeSet ? FMath::Max(0.f, StatsAttributeSet->GetPower()) : 0.f;
}

int32 AAlly::GetCharacterSpeed() const
{
	return StatsAttributeSet ? FMath::Max(0, FMath::RoundToInt(StatsAttributeSet->GetSpeed())) : 0;
}