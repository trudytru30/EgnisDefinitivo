#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EgnisPassiveAbility.generated.h"

class ACharacterBase;
class AAlly;

UCLASS(Abstract, Blueprintable)
class EGNISDEF_API UEgnisPassiveAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UEgnisPassiveAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UTexture2D> Icon;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	UFUNCTION(BlueprintPure, Category = "Ability")
	ACharacterBase* GetOwnerCharacter() const;

	UFUNCTION(BlueprintPure, Category = "Ability")
	AAlly* GetOwnerAlly() const;
};
