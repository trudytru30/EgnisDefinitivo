#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyWaitMoveEnd.generated.h"

//Espera a que el enemigo termine de andar (ponerla despues de cualquier tarea que lo mueva)
UCLASS(meta = (DisplayName = "Esperar a que termine de moverse (Egnis)"))
class EGNISDEF_API UBTTask_EnemyWaitMoveEnd : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyWaitMoveEnd();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};