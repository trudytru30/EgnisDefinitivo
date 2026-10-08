#pragma once

#include "CoreMinimal.h"
#include "HeroRuntimeData.generated.h"

class UHeroDataAsset;

USTRUCT(BlueprintType)
struct FHeroRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run State")
	UHeroDataAsset* HeroDataAsset; // Referencia al DataAsset visual

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run State")
	bool bIsUnlocked = false; // ¿Está desbloqueado en esta run?

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run State")
	float CurrentHealth = 100.f; // Vida actual durante la run
};