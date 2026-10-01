#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "HeroClassData.generated.h"

USTRUCT(BlueprintType)
struct FHeroClassData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Vitality = 100.f; // Vida máxima base

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Power = 1.0f;     // Multiplicador de cartas/efectos

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Piedad = 0.f;     // Recurso o estadística especial

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Speed = 3;        // Casillas de movimiento

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Range = 0;        // Modificador de rango

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class & Cults")
	FName CharacterClassName; // Nombre o ID de la clase (ej. "Warrior")

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class & Cults")
	TArray<FName> AllowedCults; // Cultos o tipos de cartas permitidos
};