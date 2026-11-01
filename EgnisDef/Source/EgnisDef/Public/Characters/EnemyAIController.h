#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class AEnemyAIController;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyTurnFinished, AEnemyAIController*, EnemyController);

//lleva su Behavior Tree y hace de puente con los turnos
//StartTurn() (pone bIsMyTurn = true en el Blackboard)
//el behavior Tree al acabar llama a FinishTurn() o lo hace automaticamente
UCLASS()
class EGNISDEF_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	//Nombre de tipo bool en el blackboard
	static const FName TurnKeyName;

	//termina turno y avisa
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Turn")
	FOnEnemyTurnFinished OnTurnFinished;

	//behavior tree y blackboard asignados en el BP
	UFUNCTION(BlueprintPure, Category = "Enemy|Turn")
	bool HasBrain() const;

	//pone bIsMyTurn = true
	UFUNCTION(BlueprintCallable, Category = "Enemy|Turn")
	bool StartTurn();

	//pone bIsMyTurn = false y avisa
	//Si no era su turno, no hace nada
	UFUNCTION(BlueprintCallable, Category = "Enemy|Turn")
	void FinishTurn();

	UFUNCTION(BlueprintPure, Category = "Enemy|Turn")
	bool IsTurnActive() const { return bTurnActive; }

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	bool bTurnActive = false;
};