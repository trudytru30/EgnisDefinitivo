#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EgnisEnvironmentSettings.generated.h"

// Entornos de los combates de evento (editable en Project Settings > Egnis > Entornos)
// Los valores por defecto NO son de balance: son para poder probar. Diseño los cambia.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Entornos"))
class EGNISDEF_API UEgnisEnvironmentSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEgnisEnvironmentSettings();

	// ===== Lluvia de corrupcion: mejora general de los enemigos (1 = sin cambio; en movimiento 0 = sin cambio) =====
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Lluvia de corrupcion", meta = (ClampMin = "0.1"))
	float EvilRainEnemyHealthMultiplier = 1.25f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Lluvia de corrupcion", meta = (ClampMin = "0.1"))
	float EvilRainEnemyDamageMultiplier = 1.25f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Lluvia de corrupcion")
	int32 EvilRainEnemyMoveRangeBonus = 1;

	// ===== Niebla =====
	// Si es true, los enemigos se ocultan durante el despliegue y aparecen al empezar el combate
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Niebla")
	bool bFogHidesEnemies = true;
};