#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyFinishTurn.generated.h"

//taskpara el behavior tree
UCLASS(meta = (DisplayName = "Fin de turno (Egnis)"))
class EGNISDEF_API UBTTask_EnemyFinishTurn : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyFinishTurn();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};