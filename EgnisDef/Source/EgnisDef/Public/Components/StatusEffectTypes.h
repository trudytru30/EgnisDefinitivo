#pragma once

#include "CoreMinimal.h"
#include "StatusEffectTypes.generated.h"

class ACharacterBase;

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

// Estado activo en un personaje (lo que guarda UStatusEffectComponent y lee la UI).
USTRUCT(BlueprintType)
struct FActiveStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	EStatusType Type = EStatusType::Status_Burn;

	// Turnos propios del personaje que le quedan al estado
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	int32 TurnsLeft = 0;

	// Quien lo aplico (puede ser nulo)
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	TWeakObjectPtr<ACharacterBase> Source;

	// Si el estado se aplico durante el turno del propio personaje, el final de ese mismo turno
	// no cuenta como turno consumido.
	UPROPERTY()
	bool bSkipNextDecrement = false;
};