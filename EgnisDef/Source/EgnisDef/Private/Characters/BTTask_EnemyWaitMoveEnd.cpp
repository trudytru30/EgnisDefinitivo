#include "Characters/BTTask_EnemyWaitMoveEnd.h"
#include "Characters/Enemy.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

static const AEnemy* GetTaskEnemy(const UBehaviorTreeComponent& OwnerComp)
{
	const AAIController* AI = OwnerComp.GetAIOwner();
	return AI ? Cast<AEnemy>(AI->GetPawn()) : nullptr;
}

UBTTask_EnemyWaitMoveEnd::UBTTask_EnemyWaitMoveEnd()
{
	NodeName = TEXT("Esperar a que termine de moverse (Egnis)");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_EnemyWaitMoveEnd::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AEnemy* Enemy = GetTaskEnemy(OwnerComp);
	return (IsValid(Enemy) && Enemy->IsMovingOnBoard()) ? EBTNodeResult::InProgress : EBTNodeResult::Succeeded;
}

void UBTTask_EnemyWaitMoveEnd::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const AEnemy* Enemy = GetTaskEnemy(OwnerComp);
	if (!IsValid(Enemy) || !Enemy->IsMovingOnBoard())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}