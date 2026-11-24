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
	
	// ===== Geiser (valores de prueba, NO de balance) =====
	// Cada cuantas rondas erupcionan (minimo 2). Una ronda = un turno del jugador + uno del enemigo
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Geiser", meta = (ClampMin = "2"))
	int32 GeyserEruptionPeriod = 2;

	// Cuantas rondas dura activo (bloqueado). Se limita a Periodo-1
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Geiser", meta = (ClampMin = "1"))
	int32 GeyserActiveRounds = 1;

	// Aviso la ronda antes de erupcionar (solo cambia el estado a Warning para el BP, no afecta a la jugabilidad)
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Geiser")
	bool bGeyserWarnBeforeEruption = true;

	// Daño puro fijo a quien esta encima al erupcionar
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Geiser", meta = (ClampMin = "0"))
	float GeyserDamage = 15.f;

	// Diseño dijo "aliados": por defecto los enemigos solo se empujan, no se dañan
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Geiser")
	bool bGeyserDamagesEnemies = false;

	// Solo si el nivel no define GeyserTiles: cuantos geiseres al azar
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Geiser", meta = (ClampMin = "1"))
	int32 GeyserDefaultCount = 3;
};