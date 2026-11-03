#include "Characters//EnemyAIController.h"
#include "Characters/Enemy.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

const FName AEnemyAIController::TurnKeyName(TEXT("bIsMyTurn"));

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AEnemy* Enemy = Cast<AEnemy>(InPawn);
	if (!Enemy)
	{
		return;
	}

	//El tablero coloca y mueve a los personajes casilla a casilla sin físicas,
	//para que al tener controlador no le afecte la gravedad ni el movimiento automático
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->DisableMovement();
	}

	if (!Enemy->BehaviorTree)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyAI] %s: sin Behavior Tree, usa el comportamiento antiguo."), *Enemy->GetName());
		return;
	}

	if (RunBehaviorTree(Enemy->BehaviorTree))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyAI] %s: cerebro %s en marcha."), *Enemy->GetName(), *Enemy->BehaviorTree->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] %s: no se ha podido arrancar %s (¿tiene Blackboard asignado?)."),
			*Enemy->GetName(), *Enemy->BehaviorTree->GetName());
	}
}

bool AEnemyAIController::HasBrain() const
{
	return BrainComponent && BrainComponent->IsRunning();
}

bool AEnemyAIController::StartTurn()
{
	if (!HasBrain())
	{
		return false;
	}

	UBlackboardComponent* TurnBlackboard = GetBlackboardComponent();
	if (!TurnBlackboard || TurnBlackboard->GetKeyID(TurnKeyName) == FBlackboard::InvalidKey)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] %s: su Blackboard no tiene la clave Bool '%s'."),
			*GetNameSafe(GetPawn()), *TurnKeyName.ToString());
		return false;
	}

	bTurnActive = true;
	TurnBlackboard->SetValueAsBool(TurnKeyName, true);
	UE_LOG(LogTemp, Log, TEXT("[EnemyAI] %s: empieza su turno."), *GetNameSafe(GetPawn()));

	if (AEnemy* Enemy = Cast<AEnemy>(GetPawn()))
	{
		Enemy->OnEnemyTurnStarted();
	}
	return true;
}

void AEnemyAIController::FinishTurn()
{
	if (!bTurnActive)
	{
		return;
	}

	bTurnActive = false;
	if (UBlackboardComponent* TurnBlackboard = GetBlackboardComponent())
	{
		TurnBlackboard->SetValueAsBool(TurnKeyName, false);
	}

	if (AEnemy* Enemy = Cast<AEnemy>(GetPawn()))
	{
		Enemy->OnOwnTurnFinished();
		Enemy->OnEnemyTurnEnded();
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyAI] %s: termina su turno."), *GetNameSafe(GetPawn()));
	OnTurnFinished.Broadcast(this);
}