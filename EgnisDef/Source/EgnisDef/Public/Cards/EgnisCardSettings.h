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
	
	// Tamano de la mano y del mazo (los edita diseño en Project Settings > Egnis > Cartas)
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mano", meta = (ClampMin = "1"))
	int32 InitialHandSize = 3;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mano", meta = (ClampMin = "1"))
	int32 MaxHandSize = 9;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mazo", meta = (ClampMin = "1"))
	int32 InitialDeckSize = 20;

	UFUNCTION(BlueprintPure, Category = "Card")
	static FLinearColor GetCultColor(FGameplayTag Cult);
};
