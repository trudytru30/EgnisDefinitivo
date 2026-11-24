#pragma once

#include "CoreMinimal.h"
#include "CardDisplayTypes.generated.h"

class UTexture2D;

// Si un efecto es bueno o malo para el jugador. Auto lo decide el propio efecto (ver UCardDisplayLibrary)
UENUM(BlueprintType)
enum class ECardEffectPolarity : uint8
{
	Auto UMETA(DisplayName = "Automático"),
	Good UMETA(DisplayName = "Bueno"),
	Bad  UMETA(DisplayName = "Malo")
};

// Cómo se lanza una carta, para el icono de la carta. Se deduce de sus efectos
UENUM(BlueprintType)
enum class ECardLaunchType : uint8
{
	None   UMETA(DisplayName = "Sin objetivo"),
	Self   UMETA(DisplayName = "Propio"),
	Ally   UMETA(DisplayName = "Aliado"),
	Enemy  UMETA(DisplayName = "Enemigo"),
	Tile   UMETA(DisplayName = "Casilla"),
	Area   UMETA(DisplayName = "Área")
};

// Lo que muestra la carta de cada efecto: icono, color según si es bueno o malo y su número
USTRUCT(BlueprintType)
struct EGNISDEF_API FCardEffectDisplay
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Nombre corto del efecto: se ve en lugar del icono mientras no haya uno
	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	bool bGoodForPlayer = true;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	bool bHasValue = false;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	int32 Value = 0;

	bool operator==(const FCardEffectDisplay& Other) const
	{
		return Icon == Other.Icon && Label.EqualTo(Other.Label) && bGoodForPlayer == Other.bGoodForPlayer
			&& Color == Other.Color && bHasValue == Other.bHasValue && Value == Other.Value;
	}
};

// Lo que sale en el recuadro de una palabra clave del inspector
USTRUCT(BlueprintType)
struct EGNISDEF_API FCardKeywordInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI")
	FLinearColor Color = FLinearColor::White;
};
