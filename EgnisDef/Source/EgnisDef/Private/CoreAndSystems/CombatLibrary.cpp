#include "CoreAndSystems/CombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "Characters/Ally.h"
#include "Cards/BaseCard.h"
#include "CoreAndSystems/Board.h"

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
	return !HasStatusTag(Character, TAG_Status_Stunned);
}

bool UEgnisCombatLibrary::CanMove(const ACharacterBase* Character)
{
	return !HasStatusTag(Character, TAG_Status_Paralyzed);
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