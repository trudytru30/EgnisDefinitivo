#include "Components/StatusEffectComponent.h"
#include "AbilitySystemComponent.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Characters/Ally.h"

namespace
{
	FGameplayTag GetTagForStatus(EStatusType Type)
	{
		switch (Type)
		{
		case EStatusType::Status_Burn:      return TAG_Status_Burn;
		case EStatusType::Status_Weakened:  return TAG_Status_Weakened;
		case EStatusType::Status_Exposed:   return TAG_Status_Exposed;
		case EStatusType::Status_Paralyzed: return TAG_Status_Paralyzed;
		case EStatusType::Status_Stunned:   return TAG_Status_Stunned;
		case EStatusType::Status_Bleed:     return TAG_Status_Bleed;
		case EStatusType::Status_Taunt:     return TAG_Status_Taunt;
		case EStatusType::Status_Fatigued:   return TAG_Status_Fatigued;
		case EStatusType::Status_Thorns:     return TAG_Status_Thorns;
		case EStatusType::Status_FieryCurse: return TAG_Status_FieryCurse;
		case EStatusType::Status_Rooted:     return TAG_Status_Rooted;
		case EStatusType::Status_Riposte:    return TAG_Status_Riposte;
		case EStatusType::Status_UnrestrainedRage: return TAG_Status_UnrestrainedRage;
		case EStatusType::Status_Slowed: return TAG_Status_Slowed;
		case EStatusType::Status_Ecstatic: return TAG_Status_Ecstatic;
		case EStatusType::Status_Elusive: return TAG_Status_Elusive;
		default:                            return FGameplayTag();
		}
	}
	
	// Estados que duran todo el combate: no gastan turnos y no caducan
	bool IsPermanentStatus(EStatusType Type)
	{
		return Type == EStatusType::Status_FieryCurse;
	}
	
	// Estados que se van al final del turno en que se aplican: ese turno ya cuenta como uno de los suyos
	bool CountsApplicationTurnByDefault(EStatusType Type)
	{
		return Type == EStatusType::Status_Ecstatic;
	}

	// Valor numerico que se fija al aplicar el estado
	float GetMagnitudeFor(EStatusType Type, const ACharacterBase* Source)
	{
		if (Type == EStatusType::Status_Ecstatic)
		{
			// Bonus al poder = piedad de quien lo lanza, tal como esta al aplicarlo
			const AAlly* AllySource = Cast<AAlly>(Source);
			return AllySource ? AllySource->GetCharacterPiety() : 0.f;
		}
		return 0.f;
	}
	
	// Estados que se acumulan: Cada aplicacion es una entrada aparte con su propio valor y su propia duracion
	bool IsStackableStatus(EStatusType Type)
	{
		return Type == EStatusType::Status_Ecstatic;
	}
}

UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UStatusEffectComponent::GetOwnerASC() const
{
	const ACharacterBase* Character = Cast<ACharacterBase>(GetOwner());
	return Character ? Character->GetAbilitySystemComponent() : nullptr;
}

FActiveStatus* UStatusEffectComponent::FindStatus(EStatusType Type)
{
	return ActiveStatuses.FindByPredicate([Type](const FActiveStatus& S) { return S.Type == Type; });
}

const FActiveStatus* UStatusEffectComponent::FindStatus(EStatusType Type) const
{
	return ActiveStatuses.FindByPredicate([Type](const FActiveStatus& S) { return S.Type == Type; });
}

bool UStatusEffectComponent::HasStatus(EStatusType Type) const
{
	return FindStatus(Type) != nullptr;
}

int32 UStatusEffectComponent::GetTurnsLeft(EStatusType Type) const
{
	const FActiveStatus* Found = FindStatus(Type);
	return Found ? Found->TurnsLeft : 0;
}

int32 UStatusEffectComponent::GetStacks(EStatusType Type) const
{
	const FActiveStatus* Found = FindStatus(Type);
	return Found ? Found->Stacks : 0;
}

ACharacterBase* UStatusEffectComponent::GetStatusSource(EStatusType Type) const
{
	const FActiveStatus* Found = FindStatus(Type);
	return Found ? Found->Source.Get() : nullptr;
}

bool UStatusEffectComponent::ApplyStatus(EStatusType Type, int32 Turns, ACharacterBase* Source,	EStatusApplicationTurn ApplicationTurn)
{
	
	// Maldicion Ignea: no usa turnos; cada llamada intenta aplicar una carga
	if (Type == EStatusType::Status_FieryCurse)
	{
		return ApplyFieryCurse(1, Source) > 0;
	}
	
	if (Turns <= 0 || !IsValid(GetOwner()))
	{
		return false;
	}

	// Si el turno de aplicacion cuenta, el final de ese mismo turno ya gasta uno de los Turns
	const bool bCountsFirstTurn = ApplicationTurn == EStatusApplicationTurn::Counts
		|| (ApplicationTurn == EStatusApplicationTurn::Default && CountsApplicationTurnByDefault(Type));
	const bool bSkipDecrement = bOwnerTurnActive && !bCountsFirstTurn;

	// Los estados apilables (Extasiado) no se fusionan: cada aplicacion es una entrada nueva
	if (!IsStackableStatus(Type))
	{
		if (FActiveStatus* Existing = FindStatus(Type))
		{
			// Ya activo: solo se refresca si la nueva duracion es mayor
			if (Turns > Existing->TurnsLeft)
			{
				Existing->TurnsLeft = Turns;
				Existing->bSkipNextDecrement = bSkipDecrement;
				Existing->Source = Source;
				OnStatusChanged.Broadcast();
			}
			return true;
		}
	}

	FActiveStatus NewStatus;
	NewStatus.Type = Type;
	NewStatus.TurnsLeft = Turns;
	NewStatus.Source = Source;
	NewStatus.Magnitude = GetMagnitudeFor(Type, Source);
	NewStatus.bSkipNextDecrement = bSkipDecrement;
	ActiveStatuses.Add(NewStatus);

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		ASC->AddLooseGameplayTag(GetTagForStatus(Type));
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Estado aplicado: %d (%d turnos, valor %.1f)"), *GetOwner()->GetName(), (int32)Type, Turns, NewStatus.Magnitude);
	OnStatusChanged.Broadcast();
	return true;
}

int32 UStatusEffectComponent::ApplyFieryCurse(int32 Charges, ACharacterBase* Source)
{
	ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwner());
	if (Charges <= 0 || !IsValid(OwnerChar))
	{
		return 0;
	}

	int32 Applied = 0;
	for (int32 i = 0; i < Charges; ++i)
	{
		// Cada carga hace su propia tirada, con la vida que tiene el objetivo en ese momento
		const float MaxHealth = OwnerChar->GetMaxHealth();
		const float MissingFraction = MaxHealth > 0.f
			? 1.f - FMath::Clamp(OwnerChar->GetCurrentHealth() / MaxHealth, 0.f, 1.f)
			: 0.f;
		const float Chance = FMath::Clamp(CurseMinChance + MissingFraction, 0.f, CurseMaxChance);

		if (FMath::FRand() >= Chance)
		{
			UE_LOG(LogTemp, Log, TEXT("[%s] Maldicion Ignea: carga fallada (probabilidad %.0f%%, vida %.0f/%.0f)"),
				*OwnerChar->GetName(), Chance * 100.f, OwnerChar->GetCurrentHealth(), MaxHealth);
			continue;
		}

		int32 Stacks = 1;
		if (FActiveStatus* Existing = FindStatus(EStatusType::Status_FieryCurse))
		{
			Existing->Stacks++;
			Existing->Source = Source;
			Stacks = Existing->Stacks;
		}
		else
		{
			// TurnsLeft a 0: no caduca por turnos (ver IsPermanentStatus)
			FActiveStatus NewStatus;
			NewStatus.Type = EStatusType::Status_FieryCurse;
			NewStatus.TurnsLeft = 0;
			NewStatus.Stacks = 1;
			NewStatus.Source = Source;
			ActiveStatuses.Add(NewStatus);

			if (UAbilitySystemComponent* ASC = GetOwnerASC())
			{
				ASC->AddLooseGameplayTag(TAG_Status_FieryCurse);
			}
		}

		Applied++;
		UE_LOG(LogTemp, Log, TEXT("[%s] Maldicion Ignea: carga aplicada (probabilidad %.0f%%), cargas: %d/%d"),
			*OwnerChar->GetName(), Chance * 100.f, Stacks, CurseChargesToKill);
		OnStatusChanged.Broadcast();

		if (Stacks >= CurseChargesToKill)
		{
			// Muerte directa: ignora escudo, Debilidad y Expuesto. El objetivo se destruye, asi que no se toca nada mas
			UE_LOG(LogTemp, Log, TEXT("[%s] Maldicion Ignea: %d cargas, muere"), *OwnerChar->GetName(), Stacks);
			UEgnisCombatLibrary::ApplyPureDamage(OwnerChar, OwnerChar->GetCurrentHealth());
			break;
		}
	}

	return Applied;
}

void UStatusEffectComponent::RemoveStatus(EStatusType Type)
{
	bool bRemoved = false;
	for (int32 i = ActiveStatuses.Num() - 1; i >= 0; --i)
	{
		if (ActiveStatuses[i].Type != Type)
		{
			continue;
		}

		ActiveStatuses.RemoveAt(i);
		if (UAbilitySystemComponent* ASC = GetOwnerASC())
		{
			ASC->RemoveLooseGameplayTag(GetTagForStatus(Type));
		}
		bRemoved = true;
	}

	if (bRemoved)
	{
		OnStatusChanged.Broadcast();
	}
}

void UStatusEffectComponent::OnOwnerTurnStart()
{
	bOwnerTurnActive = true;

	// Quemadura: hace su daño al principio del turno (y caduca al final)
	if (HasStatus(EStatusType::Status_Burn))
	{
		ApplyBurnDamage();
		if (!IsValid(GetOwner())) return; // murió por la quemadura
	}
	//Sangrado: igual que la quemadura, daño puro al empezar su turno
	if (HasStatus(EStatusType::Status_Bleed))
	{
		if (ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwner()))
		{
			UE_LOG(LogTemp, Log, TEXT("[%s] Sangrado: %.0f de daño puro"), *OwnerChar->GetName(), BleedDamage);
			UEgnisCombatLibrary::ApplyPureDamage(OwnerChar, BleedDamage);
		}
		if (!IsValid(GetOwner())) return; // murió por el sangrado
	}
	
	// Enraizado: regenera un % de la vida maxima al principio del turno; el % baja un valor fijo por cada turno que ya ha curado
	if (FActiveStatus* Rooted = FindStatus(EStatusType::Status_Rooted))
	{
		if (ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwner()))
		{
			const float Percent = FMath::Max(0.f, RootedInitialHealPercent - Rooted->TurnsHealed * RootedHealReductionPerTurn);
			const float HealAmount = Percent > 0.f
				? FMath::Max(1.f, FMath::RoundToFloat(OwnerChar->GetMaxHealth() * Percent))
				: 0.f;

			UE_LOG(LogTemp, Log, TEXT("[%s] Enraizado: cura %.0f (%.0f%% de %.0f), turno curado %d"),
				*OwnerChar->GetName(), HealAmount, Percent * 100.f, OwnerChar->GetMaxHealth(), Rooted->TurnsHealed + 1);

			UEgnisCombatLibrary::ApplyPureHeal(OwnerChar, HealAmount);
			Rooted->TurnsHealed++;
		}
	}
	
	// Aqui iran los efectos periodicos, antes de descontar el turno
}

void UStatusEffectComponent::OnOwnerTurnEnd()
{
	bOwnerTurnActive = false;
	ConsumeTurns();
}

void UStatusEffectComponent::ConsumeTurns()
{
	// Se recorre al reves porque los estados caducados se quitan del array durante el bucle
	bool bChanged = false;
	for (int32 i = ActiveStatuses.Num() - 1; i >= 0; --i)
	{
		FActiveStatus& Status = ActiveStatuses[i];
		
		
		// Los estados permanentes (Maldicion Ignea) duran todo el combate: no gastan turnos
		if (IsPermanentStatus(Status.Type))
		{
			continue;
		}

		if (Status.bSkipNextDecrement)
		{
			Status.bSkipNextDecrement = false;
			continue;
		}

		Status.TurnsLeft--;
		bChanged = true;

		if (Status.TurnsLeft <= 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[%s] Estado caducado: %d"), *GetOwner()->GetName(), (int32)Status.Type);

			if (UAbilitySystemComponent* ASC = GetOwnerASC())
			{
				ASC->RemoveLooseGameplayTag(GetTagForStatus(Status.Type));
			}
			ActiveStatuses.RemoveAt(i);
		}
	}

	if (bChanged)
	{
		OnStatusChanged.Broadcast();
	}
}

void UStatusEffectComponent::ApplyBurnDamage()
{
	ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwner());
	if (!OwnerChar) return;

	UE_LOG(LogTemp, Log, TEXT("[%s] Quemadura: %.0f de daño puro"), *OwnerChar->GetName(), BurnDamage);
	UEgnisCombatLibrary::ApplyPureDamage(OwnerChar, BurnDamage);
}

float UStatusEffectComponent::GetStatusMagnitude(EStatusType Type) const
{
	float Total = 0.f;
	for (const FActiveStatus& Status : ActiveStatuses)
	{
		if (Status.Type == Type)
		{
			Total += Status.Magnitude;
		}
	}
	return Total;
}