#include "Characters/BTTask_EnemyMoveToLocation.h"
#include "Characters/Enemy.h"
#include "Characters/EnemyCombatLibrary.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

UBTTask_EnemyMoveToLocation::UBTTask_EnemyMoveToLocation()
{
	NodeName = TEXT("Mover a ubicacion (Egnis)");
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyMoveToLocation, TargetLocationKey));
	bNotifyTick = true;
}

void UBTTask_EnemyMoveToLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_EnemyMoveToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AI = OwnerComp.GetAIOwner();
	AEnemy* Enemy = AI ? Cast<AEnemy>(AI->GetPawn()) : nullptr;
	const UBlackboardComponent* TaskBlackboard = OwnerComp.GetBlackboardComponent();
	if (!Enemy || !TaskBlackboard)
	{
		return EBTNodeResult::Failed;
	}

	const FVector Location = TaskBlackboard->GetValueAsVector(TargetLocationKey.SelectedKeyName);
	if (!UEnemyCombatLibrary::EnemyMoveToLocation(Enemy, Location))
	{
		return EBTNodeResult::Failed;
	}
	
	return (IsValid(Enemy) && Enemy->IsMovingOnBoard()) ? EBTNodeResult::InProgress : EBTNodeResult::Succeeded;
}


void UBTTask_EnemyMoveToLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const AAIController* AI = OwnerComp.GetAIOwner();
	const AEnemy* Enemy = AI ? Cast<AEnemy>(AI->GetPawn()) : nullptr;
	if (!IsValid(Enemy) || !Enemy->IsMovingOnBoard())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
FString UBTTask_EnemyMoveToLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Destino: %s"), *TargetLocationKey.SelectedKeyName.ToString());
}