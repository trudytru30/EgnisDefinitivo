#include "Characters//BTTask_EnemyFinishTurn.h"
#include "Characters//EnemyAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_EnemyFinishTurn::UBTTask_EnemyFinishTurn()
{
	NodeName = TEXT("Fin de turno (Egnis)");
}

EBTNodeResult::Type UBTTask_EnemyFinishTurn::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* Controller = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!Controller)
	{
		return EBTNodeResult::Failed;
	}

	Controller->FinishTurn();
	return EBTNodeResult::Succeeded;
}