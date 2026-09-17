#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class ECardTarget : uint8
{
	None,
	Ally,
	Enemy,
	Self,
	Tile
};