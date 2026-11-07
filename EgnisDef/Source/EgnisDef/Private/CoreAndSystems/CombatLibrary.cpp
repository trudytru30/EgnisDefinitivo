#include "CoreAndSystems/CombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "Characters/Ally.h"
#include "Cards/BaseCard.h"
#include "CoreAndSystems/Board.h"
#include "Components/StatusEffectComponent.h"
#include "Characters/Enemy.h"
#include "Abilities/EgnisAbilityLibrary.h"

namespace
{
	bool HasStatusTag(const ACharacterBase* Character, const FGameplayTag& Tag)
	{
		if (!Character)
		{
			return false;
		}
		const UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
		return ASC && ASC->HasMatchingGameplayTag(Tag);
	}
}

float UEgnisCombatLibrary::CalculateDamage(const ACharacterBase* Source, const ACharacterBase* Target, float BaseDamage)
{
	float Damage = BaseDamage;

	// Debilidad: el atacante hace la mitad de daño
	if (HasStatusTag(Source, TAG_Status_Weakened))
	{
		Damage *= 0.5f;
	}

	// Expuesto: el objetivo recibe el doble de daño
	if (HasStatusTag(Target, TAG_Status_Exposed))
	{
		Damage *= 2.f;
	}

	if (BaseDamage > 0.f)
	{
		Damage = FMath::Max(1.f, FMath::RoundToFloat(Damage));
	}
	
	// Debug log
	UE_LOG(LogTemp, Log, TEXT("CalculateDamage: base %.1f -> final %.1f (objetivo %s)"),
		BaseDamage, Damage, Target ? *Target->GetName() : TEXT("null"));
	return Damage;
}

float UEgnisCombatLibrary::GetThornsDamage(const ACharacterBase* Target, float DamageDealt)
{
	if (DamageDealt <= 0.f || !HasStatusTag(Target, TAG_Status_Thorns))
	{
		return 0.f;
	}

	const UStatusEffectComponent* Status = Target->StatusComp;
	const float Fraction = Status ? Status->ThornsReflectFraction : 0.5f;
	const bool bRoundUp = Status ? Status->bThornsRoundUp : true;

	// El margen pequeño evita que un error de decimales (p. ej. 3.0000001) suba o baje un punto de más
	const float Raw = DamageDealt * Fraction;
	const float Reflected = bRoundUp ? FMath::CeilToFloat(Raw - UE_KINDA_SMALL_NUMBER)
									 : FMath::FloorToFloat(Raw + UE_KINDA_SMALL_NUMBER);

	UE_LOG(LogTemp, Log, TEXT("[Thorns] %s recibe %.0f y refleja %.0f"), *Target->GetName(), DamageDealt, Reflected);
	return Reflected;
}

float UEgnisCombatLibrary::GetAttackPower(const ACharacterBase* Character)
{
	if (const AAlly* Ally = Cast<AAlly>(Character)) return Ally->GetCharacterPower();
	if (const AEnemy* Enemy = Cast<AEnemy>(Character)) return Enemy->GetPower();
	return 0.f;
}

void UEgnisCombatLibrary::ApplyPureDamage(ACharacterBase* Target, float Amount)
{
	if (!IsValid(Target) || Amount <= 0.f) return;

	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent();
	if (!ASC) return;

	// SetNumericAttributeBase no pasa por PreGameplayEffectExecute (no toca el escudo)
	// ni por PostGameplayEffectExecute (no mata), pero PreAttributeChange sigue acotando la vida a [0, Max]
	const float NewHealth = FMath::Max(0.f, Target->GetCurrentHealth() - Amount);
	ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), NewHealth);

	if (NewHealth <= 0.f)
	{
		Target->HandleDeath();
	}
}

bool UEgnisCombatLibrary::CanAct(const ACharacterBase* Character)
{
	if (const AAlly* Ally = Cast<AAlly>(Character); Ally && Ally->IsSummoningSick())
	{
		return false;
	}
	return !HasStatusTag(Character, TAG_Status_Stunned);
}

bool UEgnisCombatLibrary::CanMove(const ACharacterBase* Character)
{
	if (const AAlly* Ally = Cast<AAlly>(Character); Ally && Ally->IsSummoningSick())
	{
		return false;
	}
	return !HasStatusTag(Character, TAG_Status_Paralyzed);
}

int32 UEgnisCombatLibrary::GetEffectiveCost(const UBaseCard* Card, const ACharacterBase* Caster)
{
	if (!Card)
	{
		return 0;
	}

	int32 Cost = Card->GetCost();

	// Fatigado: las cartas cuestan mas para este personaje
	if (HasStatusTag(Caster, TAG_Status_Fatigued))
	{
		const int32 Multiplier = (Caster && Caster->StatusComp) ? Caster->StatusComp->FatigueCostMultiplier : 2;
		Cost *= Multiplier;
	}
	return Cost;
}

bool UEgnisCombatLibrary::IsCardTargetInRange(const UBaseCard* Card, const AAlly* Caster, const ACharacterBase* TargetCharacter, const FVector& Location)
{
	if (!Card || !Caster)
	{
		return false;
	}

	const ECardTarget CardTarget = Card->GetTarget();
	if (CardTarget != ECardTarget::Ally && CardTarget != ECardTarget::Enemy && CardTarget != ECardTarget::Tile)
	{
		return true;
	}

	if (CardTarget != ECardTarget::Tile && UEgnisAbilityLibrary::HasRule(Caster, TAG_Rule_Targeting_Seeding))
	{
		return true;
	}

	//Taunt
	if (CardTarget == ECardTarget::Enemy && Caster->StatusComp)
	{
		const ACharacterBase* Taunter = Caster->StatusComp->GetStatusSource(EStatusType::Status_Taunt);
		if (IsValid(Taunter) && Taunter->GetCurrentHealth() > 0.f && TargetCharacter != Taunter)
		{
			UE_LOG(LogTemp, Log, TEXT("[Taunt] %s esta provocado por %s: solo puede apuntarle a el."),
				*Caster->GetName(), *Taunter->GetName());
			return false;
		}
	}
	
	if (!Caster->Board)
	{
		return false;
	}

	FTileCoord TargetTile;
	if (CardTarget == ECardTarget::Tile)
	{
		if (!Caster->Board->WorldPointToTile(Location, TargetTile))
		{
			return false;
		}
	}
	else
	{
		if (TargetCharacter)
		{
			TargetTile = TargetCharacter->CurrentTile;
		}
		else if (CardTarget == ECardTarget::Enemy && Card->CanTargetObstacle())
		{
			// Obstaculo: no hay unidad, la casilla sale de Location
			if (!Caster->Board->WorldPointToTile(Location, TargetTile)) return false;
		}
		else
		{
			return false;
		}
	}

	const int32 Distance = FMath::Abs(TargetTile.X - Caster->CurrentTile.X) + FMath::Abs(TargetTile.Y - Caster->CurrentTile.Y);
	if (Distance > Caster->GetEffectiveRange(Card))
	{
		return false;
	}

	// Casilla que algun efecto no acepta (p. ej. ocupada, al crear un obstaculo): se rechaza antes de gastar la carta
	if (CardTarget == ECardTarget::Tile && !Card->CanTargetTile(Caster, TargetTile))
	{
		return false;
	}
	return true;
}