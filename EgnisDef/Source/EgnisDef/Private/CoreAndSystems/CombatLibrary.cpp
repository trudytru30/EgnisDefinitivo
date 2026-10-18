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

	// Expuesto: el objetivo recibe el doble de daño
	if (HasStatusTag(Target, TAG_Status_Exposed))
	{
		Damage *= 2.f;
	}

	if (BaseDamage > 0.f)
	{
		Damage = FMath::Max(1.f, FMath::RoundToFloat(Damage));
	}
	return Damage;
}