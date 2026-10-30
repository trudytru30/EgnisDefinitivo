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

	//
	//nombre de la ficha del héroe (ej. "DA_Tothüm")
	//identifica al héroe aunque haya varios de la misma clase
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run State")
	FName HeroId;

	//heridas actuales (máx EgnisRun::MaxWoundsPerHero)
	//mirar EgnisRun::GetAllWoundIds().
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run State")
	TArray<FName> Wounds;

	//reliquias equipadas (máx EgnisRun::MaxRelicsPerHero)
	//son nombre , las reequias todavía no estan programadas
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run State")
	TArray<FName> EquippedRelics;
};