#pragma once

#include "CoreMinimal.h"
#include "ColorType.generated.h"

UENUM(BlueprintType)
enum class EColorType : uint8
{
	Grey UMETA(DisplayName = "Grey"),
	Turquoise UMETA(DisplayName = "Turquoise"),
	Indigo UMETA(DisplayName = "Indigo"),
	DarkMagenta UMETA(DisplayName = "Dark Magenta"),
	Vermillion UMETA(DisplayName = "Vermillion")
};
