#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyArcheTypeDataAsset.generated.h"

class UActionDataAsset;

UENUM(BlueprintType)
enum class EEnemyArchetype : uint8
{
	Warrior UMETA(DisplayName="Warrior"),
	Healer  UMETA(DisplayName="Healer"),
};


UCLASS()
class EGNISDEF_API UEnemyArchetypeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy")
	EEnemyArchetype Archetype = EEnemyArchetype::Warrior;

	// Acciones disponibles para este enemigo (por ejemplo Attack y/o Heal)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Actions")
	TArray<TObjectPtr<UActionDataAsset>> Actions;

	// Solo para healer
	// si un aliado tiene vida por debajo de este porcentaje, el healer prioriza curarlo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Healer", meta=(ClampMin="0.0", ClampMax="1.0"))
	float HealPriorityHealthRatio = 0.6f;
};