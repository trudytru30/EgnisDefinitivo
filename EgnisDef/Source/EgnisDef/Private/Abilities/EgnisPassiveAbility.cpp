#include "Abilities/EgnisPassiveAbility.h"
#include "AbilitySystemComponent.h"
#include "Characters/Ally.h"

UEgnisPassiveAbility::UEgnisPassiveAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UEgnisPassiveAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && !Spec.IsActive())
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

ACharacterBase* UEgnisPassiveAbility::GetOwnerCharacter() const
{
	return Cast<ACharacterBase>(GetAvatarActorFromActorInfo());
}

AAlly* UEgnisPassiveAbility::GetOwnerAlly() const
{
	return Cast<AAlly>(GetAvatarActorFromActorInfo());
}
