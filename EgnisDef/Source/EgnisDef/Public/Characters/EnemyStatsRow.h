#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyStatsRow.generated.h"

//EnemyDB del excel
//
USTRUCT(BlueprintType)
struct EGNISDEF_API FEnemyStatsRow : public FTableRowBase
{
	GENERATED_BODY()

	//Vida maxima
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Life = 50.f;

	//Poder base de ataque, escudo y cura(se aplica lo de la tirada 33/66/100)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Power = 6.f;

	// Velocidad(si tiene decimales, se redondea hacia arriba)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Speed = 4.f;

	//Rango de ataque en casillas(rombo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Range = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FName Zone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	int32 Difficulty = 1;
};