#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "EgnisCardSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Cartas"))
class EGNISDEF_API UEgnisCardSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEgnisCardSettings();

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cultos", meta = (Categories = "Cult"))
	TMap<FGameplayTag, FLinearColor> CultColors;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Cultos")
	FLinearColor CommonCardColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

	UFUNCTION(BlueprintPure, Category = "Card")
	static FLinearColor GetCultColor(FGameplayTag Cult);
};
