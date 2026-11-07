#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Camera/CameraShakeBase.h"
#include "EgnisCameraSettings.generated.h"

// Efectos de camara del combate (editable en Project Settings > Egnis > Camara)
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Camara"))
class EGNISDEF_API UEgnisCameraSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEgnisCameraSettings();

	// ===== Balanceo segun el tiempo restante del combate =====
	// TSoftClassPtr: el Blueprint no se carga hasta que se usa, y no hay rutas hardcodeadas
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Balanceo por tiempo")
	TSoftClassPtr<UCameraShakeBase> TimeSwayLow;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Balanceo por tiempo")
	TSoftClassPtr<UCameraShakeBase> TimeSwayMed;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Balanceo por tiempo")
	TSoftClassPtr<UCameraShakeBase> TimeSwayHigh;

	// Fraccion de tiempo restante (1 = completo, 0 = se acabo) a partir de la cual entra cada nivel
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Balanceo por tiempo", meta = (ClampMin = "0", ClampMax = "1"))
	float LowSwayThreshold = 0.75f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Balanceo por tiempo", meta = (ClampMin = "0", ClampMax = "1"))
	float MedSwayThreshold = 0.5f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Balanceo por tiempo", meta = (ClampMin = "0", ClampMax = "1"))
	float HighSwayThreshold = 0.25f;

	// Cada cuantos segundos se relanza el balanceo vigente. 0 = no relanzar (para shakes de duracion indefinida)
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Balanceo por tiempo", meta = (ClampMin = "0"))
	float SwayRestartInterval = 0.f;

	// ===== Daño a aliados =====
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Daño")
	TSoftClassPtr<UCameraShakeBase> DamageShake;

	// El shake escala entre estos dos valores segun el daño recibido (iguales = sin escalado)
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Daño", meta = (ClampMin = "0"))
	float MinDamageShakeScale = 0.5f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Daño", meta = (ClampMin = "0"))
	float MaxDamageShakeScale = 1.5f;

	// Daño, como fraccion de la vida maxima del aliado, con el que el shake llega a la escala maxima
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Daño", meta = (ClampMin = "0.01", ClampMax = "1"))
	float DamageFractionForMaxShake = 0.5f;
};