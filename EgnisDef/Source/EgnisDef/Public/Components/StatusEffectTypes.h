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
	Status_Stunned,
	Status_Bleed,
	Status_Taunt,
	Status_Fatigued,
	Status_Thorns,
	Status_FieryCurse,
	Status_Rooted,
	Status_Riposte,
	Status_UnrestrainedRage,
	Status_Slowed,
	Status_Ecstatic,
	Status_Elusive,
	Status_SpiritAttraction,
	Status_Perfume
};

// Si el turno en que se aplica un estado cuenta como uno de sus turnos
UENUM(BlueprintType)
enum class EStatusApplicationTurn : uint8
{
	Default      UMETA(DisplayName = "Segun el estado"),
	Counts       UMETA(DisplayName = "Cuenta"),
	DoesNotCount UMETA(DisplayName = "No cuenta")
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
	
	// Cargas acumuladas (Maldicion Ignea). Vale 1 en los estados que no acumulan
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	int32 Stacks = 1;

	// Quien lo aplico (puede ser nulo)
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	TWeakObjectPtr<ACharacterBase> Source;

	// Si el estado se aplico durante el turno del propio personaje, el final de ese mismo turno
	// no cuenta como turno consumido.
	UPROPERTY()
	bool bSkipNextDecrement = false;
	
	// Turnos en los que el estado ya ha curado (Enraizado). Sube en 1 cada vez que cura
	// y sirve para calcular cuanto baja la curacion.
	UPROPERTY()
	int32 TurnsHealed = 0;
	
	// Valor numerico del estado, fijado al aplicarlo.
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float Magnitude = 0.f;
};