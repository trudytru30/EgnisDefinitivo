#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CombatTimerSubsystem.generated.h"

// Se lanza cuando algo pide sumar o restar tiempo al temporizador.
// Seconds positivo = suma, negativo = resta. Lo escucha el WBP Combat Timer.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatTimeModified, float, Seconds);

// Se lanza cada vez que el temporizador informa de como va.
// TimeRemaining / TotalTime puede pasar de 1 si se ha sumado tiempo extra.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatTimeRemainingChanged, float, TimeRemaining, float, TotalTime);

// Puente entre el codigo de C++ y el temporizador, que esta hecho en Blueprint y no
// depende del BattleManager. Vive en el GameInstance, asi que existe en cualquier escena.
UCLASS(BlueprintType)
class EGNISDEF_API UCombatTimerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CombatTimer")
	void ModifyTime(float Seconds);

	// El WBP Combat Timer se suscribe aqui y lo conecta a su evento AddTime.
	UPROPERTY(BlueprintAssignable, Category = "CombatTimer")
	FOnCombatTimeModified OnTimeModified;
	
	// El WBP Combat Timer llama a esto en cada actualizacion con sus variables Time Remaining y Total Time.
	// Esas variables viven en el widget y C++ no las ve, asi que es el widget quien avisa.
	UFUNCTION(BlueprintCallable, Category = "CombatTimer")
	void ReportTimeRemaining(float TimeRemaining, float TotalTime);

	// Lo escuchan los efectos de camara (y servira igual para la capa de musica de tiempo restante).
	UPROPERTY(BlueprintAssignable, Category = "CombatTimer")
	FOnCombatTimeRemainingChanged OnTimeRemainingChanged;
};