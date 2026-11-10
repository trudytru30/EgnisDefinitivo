#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EnemyTurnDirector.generated.h"

class AEnemy;
class AEnemyAIController;
class UBattleManager;

//Espera del turno enemigo(se configuran en BP_BattleSceneManager)
USTRUCT(BlueprintType)
struct EGNISDEF_API FEnemyTurnSettings
{
	GENERATED_BODY()

	//segundos de espera antes de que actue el enemigo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Turn", meta = (ClampMin = "0"))
	float DelayBeforeFirstEnemy = 0.5f;

	//segundos de espera entre un enemigo y el siguiente
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Turn", meta = (ClampMin = "0"))
	float DelayBetweenEnemies = 0.75f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Turn", meta = (ClampMin = "1"))
	float MaxSecondsPerEnemy = 15.f;
};

DECLARE_DELEGATE(FOnEnemyPhaseFinished);

// Director del turno enemigo: hace actuar a los enemigos uno a uno, con esperas entre ellos

UCLASS()
class EGNISDEF_API UEnemyTurnDirector : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UBattleManager* InBattleManager, const FEnemyTurnSettings& InSettings);


	void StartEnemyPhase(const TArray<AEnemy*>& Enemies, FOnEnemyPhaseFinished OnFinished);


	void Abort();

	bool IsRunning() const { return bRunning; }

private:
	void Schedule(float Delay);
	void ProcessNextEnemy();
	void OnEnemyDone();
	void HandleEnemyTimeout();
	void StopListening();

	UFUNCTION()
	void HandleEnemyTurnFinished(AEnemyAIController* EnemyController);

	UPROPERTY()
	TObjectPtr<UBattleManager> BattleManager;

	FEnemyTurnSettings Settings;

	// Enemigos de esta fase, en orden
	//si un enemigo muere o se desactiva, se ignora y se pasa al siguiente
	TArray<TWeakObjectPtr<AEnemy>> Queue;
	int32 CurrentIndex = 0;

	
	TWeakObjectPtr<AEnemyAIController> CurrentController;

	FOnEnemyPhaseFinished OnPhaseFinished;
	FTimerHandle StepTimer;
	FTimerHandle TimeoutTimer;
	bool bRunning = false;
};