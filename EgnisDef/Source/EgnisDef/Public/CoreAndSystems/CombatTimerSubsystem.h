#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CombatTimerSubsystem.generated.h"

// Se lanza cuando algo pide sumar o restar tiempo al temporizador.
// Seconds positivo = suma, negativo = resta. Lo escucha el WBP Combat Timer.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatTimeModified, float, Seconds);

// Puente entre el codigo de C++ y el temporizador, que esta hecho en Blueprint y no
// depende del BattleManager. Vive en el GameInstance, asi que existe en cualquier escena.
// Si no hay ningun temporizador escuchando, pedir tiempo no hace nada.
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
};