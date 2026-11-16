#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemy.h"
#include "GholMiniboss.generated.h"

struct FGameplayEventData;


//Todo lo marcado con AJUSTABLE se cambia en el BP (Class Defaults -> Ghol)
UCLASS()
class EGNISDEF_API AGholMiniboss : public AEnemy
{
	GENERATED_BODY()

public:
	AGholMiniboss();

	//STATS (x Apero)
	//En el BP, StatsRow tiene que apuntar a la fila de Apero
	//y estos numeros la multiplican

	//AJUSTABLE: vida = vida de Apero x esto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Stats", meta = (ClampMin = "0"))
	float HealthMultiplier = 4.f;

	//AJUSTABLE: daño = poder de Apero x esto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Stats", meta = (ClampMin = "0"))
	float DamageMultiplier = 4.f;

	//AJUSTABLE: movimiento = movimiento de Apero x esto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Stats", meta = (ClampMin = "0"))
	float MoveMultiplier = 2.f;

	//AJUSTABLE: rango de ataque = rango de Apero x esto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Stats", meta = (ClampMin = "0"))
	float AttackRangeMultiplier = 4.f;

	//MODO MUERTE(segunda barra)

	//AJUSTABLE: segundos que tarda en gastarse sola la segunda barra
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Modo muerte", meta = (ClampMin = "0.1"))
	float DeathModeSeconds = 30.f;

	//AJUSTABLE: la segunda barra solo se gasta durante el turno enemigo
	//(mientras el jugador elige sus acciones no baja)
	//Si se marca, tambien baja durante el turno del jugador
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Modo muerte")
	bool bDrainDuringPlayerTurn = false;

	//AJUSTABLE: vida de la segunda barra = vida maxima normal x esto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Modo muerte", meta = (ClampMin = "0.01"))
	float DeathHealthMultiplier = 1.5f;

	//AJUSTABLE: daño en modo muerte = daño normal x esto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Modo muerte", meta = (ClampMin = "0"))
	float DeathDamageMultiplier = 1.5f;

	//AJUSTABLE: movimiento en modo muerte = movimiento normal x esto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Modo muerte", meta = (ClampMin = "0"))
	float DeathMoveMultiplier = 2.f;

	//REVIVIR 

	//AJUSTABLE: vida con la que revive(60 % de su vida maxima normal)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Revivir", meta = (ClampMin = "0.01", ClampMax = "1"))
	float ReviveHealthPercent = 0.6f;

	//AJUSTABLE: cuantas veces puede revivir como maximo (0 = sin limite)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghol|Revivir", meta = (ClampMin = "0"))
	int32 MaxRevives = 0;

	//Consultas para el bt

	UFUNCTION(BlueprintPure, Category = "Ghol")
	bool IsInDeathMode() const { return bInDeathMode; }

	//veces que ha revivido en este combate
	UFUNCTION(BlueprintPure, Category = "Ghol")
	int32 GetReviveCount() const { return ReviveCount; }

	//turnos extra que le quedan en esta fase enemiga
	UFUNCTION(BlueprintPure, Category = "Ghol")
	int32 GetExtraTurnsLeft() const { return ExtraTurnsLeft; }

	//segundos que le quedan a la segunda barra si nadie le pega
	UFUNCTION(BlueprintPure, Category = "Ghol")
	float GetDeathModeSecondsLeft() const;

	// ACCIONES(pruebas) 

	//pasa a modo muerte(ocurre al quedarse sin vida)
	UFUNCTION(BlueprintCallable, Category = "Ghol")
	void GholEnterDeathMode();

	//revive (ocurre al matar a un heroe en modo muerte)
	UFUNCTION(BlueprintCallable, Category = "Ghol")
	void GholRevive();

	//gasta la segunda barra como si pasaran estos segundos
	UFUNCTION(BlueprintCallable, Category = "Ghol")
	void GholDrainDeathBar(float Seconds);

	//CONEXION PARA ANIMACIONES, UI Y EFECTOS 

	//se queda sin la primera barra y aparece la segunda
	UFUNCTION(BlueprintImplementableEvent, Category = "Ghol|Animation")
	void OnGholDeathModeStarted(float SecondBarHealth, float Seconds);

	//mata a un heroe en modo muerte y revive
	UFUNCTION(BlueprintImplementableEvent, Category = "Ghol|Animation")
	void OnGholRevived(int32 TimesRevived, float NewHealth);

	//hace un turno extra seguido
	UFUNCTION(BlueprintImplementableEvent, Category = "Ghol|Animation")
	void OnGholExtraTurn(int32 ExtraTurnsLeftAfterThis);

	//muere del todo (se acabo la segunda barra)
	UFUNCTION(BlueprintImplementableEvent, Category = "Ghol|Animation")
	void OnGholFinalDeath();

	
	virtual void Tick(float DeltaTime) override;
	virtual void HandleDeath() override;
	virtual void OnOwnTurnFinished() override;
	virtual void OnEnemyPhaseStarted() override;
	virtual bool ConsumeExtraTurn() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyGholStats();
	void SetHealthValues(float NewMax, float NewHealth);
	void HandleKillEvent(const FGameplayEventData* Payload);
	void FinalDeath();
	bool IsMyTurnRunning() const;

	//Stats normales (sin modo muerte),para poder volver a ellas al revivir
	float NormalMaxHealth = 0.f;
	float NormalPower = 0.f;
	int32 NormalMoveRange = 0;

	float SecondBarMax = 0.f;
	bool bInDeathMode = false;
	bool bPendingFinalDeath = false;
	bool bFinalDeathDone = false;

	int32 ReviveCount = 0;
	int32 ExtraTurnsNextPhase = 0;
	int32 ExtraTurnsLeft = 0;

	FDelegateHandle KillEventHandle;
};
