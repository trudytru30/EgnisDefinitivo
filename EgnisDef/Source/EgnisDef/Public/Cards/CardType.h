#pragma once

#include "CoreMinimal.h"
#include "CardType.generated.h"

UENUM(BlueprintType)	// Para que se pueda ver en el editor
enum class ECardType : uint8
{
	Attack UMETA(DisplayName = "Attack"),
	Utility UMETA(DisplayName = "Utility"),
	Heal UMETA(DisplayName = "Heal")//defense
};