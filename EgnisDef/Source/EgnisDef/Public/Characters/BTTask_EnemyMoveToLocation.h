#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyMoveToLocation.generated.h"

//Mueve al enemigo a la casilla de una ubicacion del Blackboard (p. ej. el resultado del EQS)
UCLASS(meta = (DisplayName = "Mover a ubicacion (Egnis)"))
class EGNISDEF_API UBTTask_EnemyMoveToLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyMoveToLocation();

	//Clave Vector del Blackboard con el destino
	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector TargetLocationKey;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
};