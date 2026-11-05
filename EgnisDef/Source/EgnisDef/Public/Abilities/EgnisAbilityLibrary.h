#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EgnisAbilityLibrary.generated.h"

UCLASS()
class EGNISDEF_API UEgnisAbilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Abilities|Events", meta = (AutoCreateRefTerm = "InstigatorTags", AdvancedDisplay = "Instigator,Target,Magnitude,OptionalObject,InstigatorTags"))
	static void SendEvent(AActor* Receiver, FGameplayTag EventTag, AActor* Instigator, AActor* Target,
		float Magnitude, const UObject* OptionalObject, const FGameplayTagContainer& InstigatorTags);

	UFUNCTION(BlueprintCallable, Category = "Abilities|Events", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "Instigator,Target,Magnitude,Excluded"))
	static void SendEventToAllCharacters(const UObject* WorldContextObject, FGameplayTag EventTag, AActor* Instigator = nullptr,
		AActor* Target = nullptr, float Magnitude = 0.f, const AActor* Excluded = nullptr);

	UFUNCTION(BlueprintPure, Category = "Abilities|Rules")
	static bool HasRule(const AActor* Actor, FGameplayTag RuleTag);
};
