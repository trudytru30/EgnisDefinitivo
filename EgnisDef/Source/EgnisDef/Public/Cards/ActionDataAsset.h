#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActionDataAsset.generated.h"

UENUM(BlueprintType)
enum class EActionId : uint8
{
	Attack UMETA(DisplayName = "Attack"),
	Heal UMETA(DisplayName = "Heal"),
};

UENUM(BLueprintType)
enum class ETargetType : uint8
{
	Unit UMETA(DisplayName = "Unit"),
	Tile UMETA(DisplayName = "Tile"),
	Self UMETA(DisplayName = "Self"),
};


UCLASS()
class EGNISDEF_API UActionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//Que acción es(Attack/Heal/...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Action")
	EActionId ActionId = EActionId::Attack;

	// Coste base en puntos de energía
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Action")
	int32 Cost = 1;

	// Tipo de objetivo (para futuro: UI + validación)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	ETargetType TargetType = ETargetType::Unit;

	// Rango básico en casillas (implementada para futuro, de momento el rango es fijo)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	int32 Range = 1;
};