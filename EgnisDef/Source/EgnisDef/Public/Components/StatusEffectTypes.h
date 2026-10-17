#pragma once

#include "CoreMinimal.h"
#include "StatusEffectTypes.generated.h"

// Estados alterados implementados. Cada uno tiene su GameplayTag (EgnisGameplayTags.h).
UENUM(BlueprintType)
enum class EStatusType : uint8
{
	Status_Burn,
	Status_Weakened,
	Status_Exposed,
	Status_Paralyzed,
	Status_Stunned
};