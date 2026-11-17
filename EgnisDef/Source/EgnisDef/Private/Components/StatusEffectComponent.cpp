#include "Components/StatusEffectComponent.h"
#include "AbilitySystemComponent.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/HealthAttributeSet.h"

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
		case EStatusType::Status_SpiritAttraction: return TAG_Status_SpiritAttraction;
		case EStatusType::Status_Perfume: return TAG_Status_Perfume;
		case EStatusType::Status_Madness: return TAG_Status_Madness;
		case EStatusType::Status_Frightened:   return TAG_Status_Frightened;
		case EStatusType::Status_BoilingBlood: return TAG_Status_BoilingBlood;
		case EStatusType::Status_Ashes:        return TAG_Status_Ashes;
		default:                            return FGameplayTag();
		}
	}
	
	// Estados que duran todo el combate: no gastan turnos y no caducan
	bool IsPermanentStatus(EStatusType Type)
	{
		return Type == EStatusType::Status_FieryCurse || Type == EStatusType::Status_Madness
			|| Type == EStatusType::Status_Ashes;
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

bool UStatusEffectComponent::HasStatusFromSource(EStatusType Type, const ACharacterBase* Source) const
{
	if (!Source)
	{
		return false;
	}
	return ActiveStatuses.ContainsByPredicate([Type, Source](const FActiveStatus& S)
	{
		return S.Type == Type && S.Source.Get() == Source;
	});
}

bool UStatusEffectComponent::ApplyStatus(EStatusType Type, int32 Turns, ACharacterBase* Source,	EStatusApplicationTurn ApplicationTurn)
{
	
	// Asustado e Ira irrefrenable son incompatibles: con Ira activa se es inmune a Asustado (no gasta carga de Cenizas)
	if (Type == EStatusType::Status_Frightened && HasStatus(EStatusType::Status_UnrestrainedRage))
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Asustado ignorado: es inmune por Ira irrefrenable"), *GetNameSafe(GetOwner()));
		return false;
	}

	// Perfumar solo afecta a enemigos y necesita saber quien lo lanza (es a quien ignoraran)
	if (Type == EStatusType::Status_Perfume)
	{
		ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwner());
		if (!OwnerChar || OwnerChar->GetTeam() == 0 || !IsValid(Source))
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Perfumar solo se aplica a enemigos y con lanzador, se rechaza"),
				*GetNameSafe(GetOwner()));
			return false;
		}
	}
	
	// Cenizas: bloquea cualquier estado que llegue y gasta una carga
	if (TryBlockWithAshes(Type))
	{
		return false;
	}

	// Cenizas: no usa turnos. 'Turns' son las cargas que se aplican
	if (Type == EStatusType::Status_Ashes)
	{
		return ApplyAshes(Turns, Source);
	}
	
	// Maldicion Ignea: no usa turnos; cada llamada intenta aplicar una carga
	if (Type == EStatusType::Status_FieryCurse)
	{
		return ApplyFieryCurse(1, Source) > 0;
	}
	
	// Ira irrefrenable anula Asustado si ya lo tenia (despues de Cenizas: si Cenizas bloquea la Ira, no se quita nada)
	if (Type == EStatusType::Status_UnrestrainedRage && HasStatus(EStatusType::Status_Frightened))
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Ira irrefrenable anula Asustado"), *GetNameSafe(GetOwner()));
		RemoveStatus(EStatusType::Status_Frightened);
	}
	
	if (Turns <= 0 || !IsValid(GetOwner()))
	{
		return false;
	}

	// Si el turno de aplicacion cuenta, el final de ese mismo turno ya gasta uno de los Turns
	const bool bCountsFirstTurn = ApplicationTurn == EStatusApplicationTurn::Counts
		|| (ApplicationTurn == EStatusApplicationTurn::Default && CountsApplicationTurnByDefault(Type));
	const bool bSkipDecrement = bOwnerTurnActive && !bCountsFirstTurn;

	// Perfumar: una entrada por lanzador. Varios heroes pueden perfumar al mismo enemigo, cada uno con su duracion
	if (Type == EStatusType::Status_Perfume)
	{
		if (FActiveStatus* SameSource = ActiveStatuses.FindByPredicate([Source](const FActiveStatus& S)
			{ return S.Type == EStatusType::Status_Perfume && S.Source.Get() == Source; }))
		{
			if (Turns > SameSource->TurnsLeft)
			{
				SameSource->TurnsLeft = Turns;
				SameSource->bSkipNextDecrement = bSkipDecrement;
				OnStatusChanged.Broadcast();
			}
			return true;
		}
	}

	// Los estados apilables (Extasiado) no se fusionan: cada aplicacion es una entrada nueva
	if (!IsStackableStatus(Type) && Type != EStatusType::Status_Perfume)
	{
		if (FActiveStatus* Existing = FindStatus(Type))
		{
			if (IsPermanentStatus(Type))
			{
				return true;
			}
			
			// Atraccion Espiritual: otro lanzador distinto sobrescribe al anterior, aunque dure menos
			const bool bNewProvoker = Type == EStatusType::Status_SpiritAttraction && Existing->Source.Get() != Source;

			// Ya activo: solo se refresca si la nueva duracion es mayor
			if (bNewProvoker || Turns > Existing->TurnsLeft)
			{
				Existing->TurnsLeft = Turns;
				Existing->bSkipNextDecrement = bSkipDecrement;
				Existing->Source = Source;
				if (bNewProvoker)
				{
					UE_LOG(LogTemp, Log, TEXT("[Atraccion] %s: ahora provocado por %s"), *GetOwner()->GetName(),
						Source ? *Source->GetName() : TEXT("nadie"));
				}
				OnStatusChanged.Broadcast();
			}
			return true;
		}
	}

	FActiveStatus NewStatus;
	NewStatus.Type = Type;
	NewStatus.TurnsLeft = IsPermanentStatus(Type) ? 0 : Turns;
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
	
	if (TryBlockWithAshes(EStatusType::Status_FieryCurse))
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
	
	// Escudos temporales: pierden un turno y caducan los que llegan a 0
	ExpireTimedShields();

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

bool UStatusEffectComponent::TryBlockWithAshes(EStatusType Incoming)
{
	if (Incoming == EStatusType::Status_Ashes)
	{
		return false; // aplicar mas Cenizas suma cargas, no se bloquea
	}

	FActiveStatus* Ashes = FindStatus(EStatusType::Status_Ashes);
	if (!Ashes)
	{
		return false;
	}

	Ashes->Stacks--;
	UE_LOG(LogTemp, Log, TEXT("[%s] Cenizas: bloquea el estado %d, cargas: %d"),
		*GetOwner()->GetName(), (int32)Incoming, Ashes->Stacks);

	if (Ashes->Stacks <= 0)
	{
		RemoveStatus(EStatusType::Status_Ashes); // quita la entrada y el tag y avisa a la UI
	}
	else
	{
		OnStatusChanged.Broadcast();
	}
	return true;
}

bool UStatusEffectComponent::ApplyAshes(int32 Stacks, ACharacterBase* Source)
{
	if (Stacks <= 0 || !IsValid(GetOwner()))
	{
		return false;
	}

	if (FActiveStatus* Existing = FindStatus(EStatusType::Status_Ashes))
	{
		Existing->Stacks += Stacks;
		Existing->Source = Source;
		UE_LOG(LogTemp, Log, TEXT("[%s] Cenizas: +%d, cargas: %d"), *GetOwner()->GetName(), Stacks, Existing->Stacks);
		OnStatusChanged.Broadcast();
		return true;
	}

	FActiveStatus NewStatus;
	NewStatus.Type = EStatusType::Status_Ashes;
	NewStatus.TurnsLeft = 0; // no caduca por turnos (ver IsPermanentStatus)
	NewStatus.Stacks = Stacks;
	NewStatus.Source = Source;
	ActiveStatuses.Add(NewStatus);

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		ASC->AddLooseGameplayTag(TAG_Status_Ashes);
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Cenizas aplicadas, cargas: %d"), *GetOwner()->GetName(), Stacks);
	OnStatusChanged.Broadcast();
	return true;
}

void UStatusEffectComponent::OnOwnerActionTaken(const TCHAR* ActionName)
{
	if (!HasStatus(EStatusType::Status_BoilingBlood))
	{
		return;
	}

	ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwner());
	if (!OwnerChar || BoilingBloodDamage <= 0.f)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Sangre hirviente (%s): %.0f de daño puro"), *OwnerChar->GetName(), ActionName, BoilingBloodDamage);
	UEgnisCombatLibrary::ApplyPureDamage(OwnerChar, BoilingBloodDamage);
}

void UStatusEffectComponent::AddTimedShield(float Amount, int32 Turns)
{
	if (Amount <= 0.f || Turns <= 0)
	{
		return;
	}

	FTimedShield& NewShield = TimedShields.AddDefaulted_GetRef();
	NewShield.Amount = Amount;
	NewShield.TurnsLeft = Turns;
}

void UStatusEffectComponent::ExpireTimedShields()
{
	ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (TimedShields.Num() == 0 || !IsValid(OwnerChar) || !ASC)
	{
		return;
	}

	// Lo que el escudo ha perdido desde que se dio (golpes absorbidos...) se descuenta primero de los temporales
	// que caducan antes. El escudo que no esta apuntado aqui (InitialShield, otros efectos) no caduca.
	float Tracked = 0.f;
	for (const FTimedShield& Shield : TimedShields)
	{
		Tracked += Shield.Amount;
	}
	float Missing = Tracked - OwnerChar->GetCurrentShield();
	if (Missing > 0.f)
	{
		TimedShields.StableSort([](const FTimedShield& A, const FTimedShield& B) { return A.TurnsLeft < B.TurnsLeft; });
		for (FTimedShield& Shield : TimedShields)
		{
			const float Taken = FMath::Min(Shield.Amount, Missing);
			Shield.Amount -= Taken;
			Missing -= Taken;
			if (Missing <= 0.f)
			{
				break;
			}
		}
	}

	// Pasa un turno y se quitan los que llegan a 0
	float ToRemove = 0.f;
	for (int32 i = TimedShields.Num() - 1; i >= 0; --i)
	{
		if (--TimedShields[i].TurnsLeft > 0)
		{
			continue;
		}
		ToRemove += TimedShields[i].Amount;
		TimedShields.RemoveAt(i);
	}
	if (ToRemove <= 0.f)
	{
		return;
	}

	const float Before = OwnerChar->GetCurrentShield();
	if (OwnerChar->HealthAttributeSet)
	{
		OwnerChar->HealthAttributeSet->bSilentShieldChange = true;
	}
	ASC->ApplyModToAttribute(UHealthAttributeSet::GetShieldAttribute(), EGameplayModOp::Additive, -ToRemove);
	if (OwnerChar->HealthAttributeSet)
	{
		OwnerChar->HealthAttributeSet->bSilentShieldChange = false;
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Escudo temporal caduca: %.0f (escudo %.0f -> %.0f)"),
		*OwnerChar->GetName(), ToRemove, Before, OwnerChar->GetCurrentShield());
}