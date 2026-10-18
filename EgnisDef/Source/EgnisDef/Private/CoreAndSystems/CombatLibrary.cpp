#include "CoreAndSystems/CombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/EgnisGameplayTags.h"

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

bool UEgnisCombatLibrary::CanAct(const ACharacterBase* Character)
{
	return !HasStatusTag(Character, TAG_Status_Stunned);
}

bool UEgnisCombatLibrary::CanMove(const ACharacterBase* Character)
{
	return !HasStatusTag(Character, TAG_Status_Paralyzed);
}