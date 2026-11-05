#include "Abilities/EgnisAbilityLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/CharacterBase.h"
#include "EngineUtils.h"

void UEgnisAbilityLibrary::SendEvent(AActor* Receiver, FGameplayTag EventTag, AActor* Instigator, AActor* Target,
	float Magnitude, const UObject* OptionalObject, const FGameplayTagContainer& InstigatorTags)
{
	if (!IsValid(Receiver) || !EventTag.IsValid() || !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Receiver))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Instigator;
	Payload.Target = Target;
	Payload.EventMagnitude = Magnitude;
	Payload.OptionalObject = OptionalObject;
	Payload.InstigatorTags = InstigatorTags;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Receiver, EventTag, Payload);
}

void UEgnisAbilityLibrary::SendEventToAllCharacters(const UObject* WorldContextObject, FGameplayTag EventTag, AActor* Instigator,
	AActor* Target, float Magnitude, const AActor* Excluded)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	TArray<ACharacterBase*> Characters;
	for (TActorIterator<ACharacterBase> It(World); It; ++It)
	{
		if (*It != Excluded)
		{
			Characters.Add(*It);
		}
	}

	for (ACharacterBase* Character : Characters)
	{
		SendEvent(Character, EventTag, Instigator, Target, Magnitude, nullptr, FGameplayTagContainer());
	}
}

bool UEgnisAbilityLibrary::HasRule(const AActor* Actor, FGameplayTag RuleTag)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
	return ASC && RuleTag.IsValid() && ASC->HasMatchingGameplayTag(RuleTag);
}
