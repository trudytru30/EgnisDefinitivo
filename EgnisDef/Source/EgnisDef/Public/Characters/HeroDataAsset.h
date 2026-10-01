#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeroDataAsset.generated.h"

class AAlly;
class UBaseCard;

UCLASS(BlueprintType)
class EGNISDEF_API UHeroDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// La clave exacta (Row Name) que buscará en la DataTable de Excel (ej. "Row_Warrior")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Excel Link")
	FName ExcelRowName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FText HeroName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	UTexture2D* SplashArt;

	// El modelo 3D que se spawnea en el tablero
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawning")
	TSubclassOf<AAlly> AllyBlueprintClass;

	// Cartas personales que aporta al mazo al ser seleccionado
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cards")
	TArray<TSubclassOf<UBaseCard>> PersonalCards;
};