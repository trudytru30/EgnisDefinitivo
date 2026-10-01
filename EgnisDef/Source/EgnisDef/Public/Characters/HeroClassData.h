#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "HeroClassData.generated.h"

USTRUCT(BlueprintType)
struct FHeroClassData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Vitality; // Vida máxima base

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Power;     // Multiplicador de cartas/efectos

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Piety ;     // Recurso o estadística especial

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Speed;        // Casillas de movimiento

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Range;        // Modificador de rango

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class & Cults")
	FName CharacterClassName; // Nombre o ID de la clase (ej. "Warrior")

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class & Cults")
	FGameplayTagContainer AllowedCults; // Tags de cultos
};