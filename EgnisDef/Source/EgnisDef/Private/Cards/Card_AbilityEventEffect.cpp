#include "Cards/Card_AbilityEventEffect.h"
#include "Abilities/EgnisAbilityLibrary.h"
#include "Cards/BaseCard.h"

UCard_AbilityEventEffect::UCard_AbilityEventEffect()
{
	Target = ECardTarget::Self;
	bShowOnCard = false;
}

void UCard_AbilityEventEffect::Execute_Self(ACharacterBase* Source)
{
	Super::Execute_Self(Source);

	if (!IsValid(Source) || !EventTag.IsValid())
	{
		return;
	}

	UEgnisAbilityLibrary::SendEvent(Source, EventTag, Source, nullptr, Magnitude,
		UEgnisAbilityLibrary::GetCardInPlay(Source), FGameplayTagContainer());
}
