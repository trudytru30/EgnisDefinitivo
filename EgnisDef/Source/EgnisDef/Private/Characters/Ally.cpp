#include "Characters/Ally.h"
#include "Characters/HeroDataAsset.h"
#include "CoreAndSystems/AudioManager.h"
#include "Components/GridMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "Cards/BaseCard.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreAndSystems/CharacterStatsAttributesSet.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/EgnisAbilityLibrary.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Cards/EgnisCardSettings.h"
#include "Components/StatusEffectComponent.h"
#include "FMODEvent.h"

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
	if (HeroAsset)
	{
		InitializeFromData(HeroAsset);
	}

	Super::BeginPlay();

	// Cada vez que cambie Speed (tabla, herida, bufo...) se actualiza el movimiento
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UCharacterStatsAttributeSet::GetSpeedAttribute())
			.AddUObject(this, &AAlly::OnSpeedChanged);
	}
	
	// Ralentizado: al aplicarse o caducar un estado hay que recalcular el movimiento
	if (StatusComp)
	{
		StatusComp->OnStatusChanged.AddUniqueDynamic(this, &AAlly::OnStatusesChanged);
	}
}

void AAlly::OnSpeedChanged(const FOnAttributeChangeData& Data)
{
	RefreshMoveRange();
}

void AAlly::OnStatusesChanged()
{
	RefreshMoveRange();
}

void AAlly::RefreshMoveRange()
{
	if (!MoveComp)
	{
		return;
	}

	const int32 Speed = GetCharacterSpeed();
	const int32 NewRange = UEgnisCombatLibrary::GetSlowedRange(this, Speed);
	if (NewRange != Speed)
	{
		UE_LOG(LogTemp, Log, TEXT("[Ralentizado] %s: movimiento %d -> %d"), *GetName(), Speed, NewRange);
	}

	MoveComp->moveRange = NewRange;
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

    if (MoveComp)
    {
        MoveComp->movePattern = HeroData->MovePattern;
    }

    // 3. Inicializar Estadísticas usando GAS
    if (GetAbilitySystemComponent())
    {
    }
	
	// Estadísticas del personaje desde la tabla de stats (columna Range y Piety de DT_HeroClasses)
	if (const FHeroClassData* Stats = HeroData->HeroStatsRow.GetRow<FHeroClassData>(TEXT("AAlly::InitializeFromData")))
	{
		MaxHealth = Stats->Vitality;
		Health = Stats->Vitality;

		// Power, Piety y Speed viven en GAS: la tabla da el valor base
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			ASC->SetNumericAttributeBase(UCharacterStatsAttributeSet::GetPowerAttribute(), Stats->Power);
			ASC->SetNumericAttributeBase(UCharacterStatsAttributeSet::GetPietyAttribute(), Stats->Piety);
			ASC->SetNumericAttributeBase(UCharacterStatsAttributeSet::GetSpeedAttribute(), Stats->Speed);
			ASC->SetNumericAttributeBase(UCharacterStatsAttributeSet::GetRangeAttribute(), Stats->Range);
			ASC->SetNumericAttributeBase(UHealthAttributeSet::GetMaxHealthAttribute(), Stats->Vitality);
			ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), Stats->Vitality);
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
	
	// Invocaciones: la vida que fija el efecto (piedad x multiplicador) manda sobre la Vitality de la tabla
	if (HealthOverride > 0.f)
	{
		MaxHealth = HealthOverride;
		Health = HealthOverride;
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			ASC->SetNumericAttributeBase(UHealthAttributeSet::GetMaxHealthAttribute(), HealthOverride);
			ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), HealthOverride);
		}
		UE_LOG(LogTemp, Log, TEXT("Invocacion %s: vida fijada a %.1f"), *GetName(), HealthOverride);
	}

	GrantHeroAbilities(HeroData);

    UE_LOG(LogTemp, Log, TEXT("Aliado %s inicializado desde DataAsset."), *GetName());
}

void AAlly::GrantHeroAbilities(const UHeroDataAsset* HeroData)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !HeroData)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : GrantedHeroAbilities)
	{
		ASC->ClearAbility(Handle);
	}
	GrantedHeroAbilities.Reset();

	TArray<TSubclassOf<UGameplayAbility>> Abilities;
	if (const FHeroClassData* Stats = HeroData->HeroStatsRow.GetRow<FHeroClassData>(TEXT("AAlly::GrantHeroAbilities")))
	{
		Abilities.Add(Stats->ClassAbility.LoadSynchronous());
	}
	Abilities.Add(HeroData->UniqueAbility);

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Abilities)
	{
		if (AbilityClass)
		{
			GrantedHeroAbilities.Add(ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this)));
			UE_LOG(LogTemp, Log, TEXT("Aliado %s: habilidad %s concedida"), *GetName(), *AbilityClass->GetName());
		}
	}
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

int32 AAlly::GetSecondWindEnergy() const
{
	return EnergyAttributeSet ? FMath::RoundToInt(EnergyAttributeSet->GetSecondWindEnergy()) : 0;
}

void AAlly::AddSecondWindEnergy(int32 Amount, int32 MaxAmount)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const int32 NewValue = FMath::Clamp(GetSecondWindEnergy() + Amount, 0, MaxAmount);
		ASC->SetNumericAttributeBase(UEnergyAttributeSet::GetSecondWindEnergyAttribute(), NewValue);
		UE_LOG(LogTemp, Log, TEXT("Aliado %s: segunda reserva de mana = %d"), *GetName(), NewValue);
	}
}

bool AAlly::CanPayCardCost(const UBaseCard* Card) const
{
	if (!Card)
	{
		return false;
	}

	const int32 Missing = Card->GetCost() - GetCurrentEnergy();
	if (Missing <= 0)
	{
		return true;
	}

	if (UEgnisAbilityLibrary::HasRule(this, TAG_Rule_Cost_SecondWind) && GetSecondWindEnergy() >= Missing)
	{
		return true;
	}

	return UEgnisAbilityLibrary::HasRule(this, TAG_Rule_Cost_VitalDebt) && GetCurrentEnergy() > 0 && GetCurrentHealth() > Missing;
}

void AAlly::PayCardCost(const UBaseCard* Card)
{
	if (!Card)
	{
		return;
	}

	const int32 Cost = Card->GetCost();
	const int32 EnergyPaid = FMath::Min(Cost, GetCurrentEnergy());
	LossPoints(EnergyPaid);

	const int32 Missing = Cost - EnergyPaid;
	if (Missing <= 0)
	{
		return;
	}

	if (UEgnisAbilityLibrary::HasRule(this, TAG_Rule_Cost_SecondWind) && GetSecondWindEnergy() >= Missing)
	{
		AddSecondWindEnergy(-Missing, GetSecondWindEnergy());
		return;
	}

	UEgnisCombatLibrary::ApplyPureDamage(this, Missing);
	UE_LOG(LogTemp, Log, TEXT("Aliado %s: paga %d de vida por %s"), *GetName(), Missing, *Card->GetName());
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

bool AAlly::CounterAttack(ACharacterBase* Target)
{
	if (!IsValid(Target) || Target->GetTeam() == GetTeam())
	{
		return false;
	}

	UBaseCard* RiposteCard = GetDefault<UEgnisCardSettings>()->RiposteCard.LoadSynchronous();
	if (!RiposteCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Riposte: no hay RiposteCard en Project Settings > Egnis > Cartas."), *GetName());
		return false;
	}

	const int32 Distance = FMath::Abs(Target->CurrentTile.X - CurrentTile.X) + FMath::Abs(Target->CurrentTile.Y - CurrentTile.Y);
	if (Distance > GetEffectiveRange(RiposteCard))
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Riposte: %s esta fuera de su alcance (%d), no responde."),
			*GetName(), *Target->GetName(), GetEffectiveRange(RiposteCard));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Riposte -> %s con %s"), *GetName(), *Target->GetName(), *RiposteCard->GetName());
	RiposteCard->ExecuteAgainst(this, Target);
	return true;
}

UFMODEvent* AAlly::GetAttackSoundEvent() const
{
	return HeroAsset ? HeroAsset->AttackSound : nullptr;
}

float AAlly::GetAudioPitch() const
{
	return HeroAsset ? HeroAsset->PitchModifier : 1.f;
}