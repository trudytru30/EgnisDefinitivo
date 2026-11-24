#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Cards/CardDisplayTypes.h"
#include "Components/StatusEffectTypes.h"
#include "CardDisplaySettings.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct EGNISDEF_API FCardStatusDisplay
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI")
	TSoftObjectPtr<UTexture2D> Icon;

	// Beneficio para quien lo recibe: a un aliado es bueno y a un enemigo es malo. Un perjuicio, al revés
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI")
	bool bBeneficial = false;
};

// Palabra clave de las descripciones: sale de color con su icono detrás y tiene su recuadro en el inspector.
// Estos Id los usa el código para enlazar los efectos de las cartas con su palabra: Damage, Heal, Shield, Push,
// Pull, Move, Draw, Discard, Consume, Keep, Obstacle, Spirit, Time, Ephemeral, PlaysTwice y EnergyOnKill.
// Los estados se enlazan con Status
USTRUCT(BlueprintType)
struct EGNISDEF_API FCardKeyword
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword")
	FText Name;

	// Palabras de la descripción que la marcan solas, sin distinguir mayúsculas. Vacío: no se marca sola, solo
	// sale su recuadro si la carta tiene el efecto. A mano se puede marcar con [Nombre] en la descripción
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword")
	TArray<FString> Words;

	// Explicación que sale en su recuadro del inspector
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword", meta = (MultiLine = true))
	FText Description;

	// Si es un estado, coge el icono y si es bueno o malo de la lista de Estados
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword")
	bool bIsStatus = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword", meta = (EditCondition = "bIsStatus"))
	EStatusType Status = EStatusType::Status_Burn;

	// Si se deja vacío y es un estado, usa el del estado
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword")
	TSoftObjectPtr<UTexture2D> Icon;

	// Automático: bueno o malo según el estado; si no es un estado, sale con el color de palabra clave
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword")
	ECardEffectPolarity Polarity = ECardEffectPolarity::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword", meta = (InlineEditConditionToggle))
	bool bOverrideColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword", meta = (EditCondition = "bOverrideColor"))
	FLinearColor Color = FLinearColor::White;
};

// Iconos y colores de los efectos que se ven en las cartas (Project Settings > Egnis > Iconos de cartas).
// El icono de cada efecto va en los valores por defecto de su BP_*Effect; aquí van los que no tienen BP propio
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Iconos de cartas"))
class EGNISDEF_API UEgnisCardDisplaySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEgnisCardDisplaySettings();

	// Los iconos se tiñen con estos colores: conviene dibujarlos en blanco o gris
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Colores")
	FLinearColor GoodColor = FLinearColor(0.35f, 0.85f, 0.45f, 1.f);

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Colores")
	FLinearColor BadColor = FLinearColor(0.90f, 0.25f, 0.25f, 1.f);

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Colores")
	FLinearColor LaunchTypeColor = FLinearColor::White;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Lanzamiento")
	TMap<ECardLaunchType, TSoftObjectPtr<UTexture2D>> LaunchTypeIcons;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Estados")
	TMap<EStatusType, FCardStatusDisplay> Statuses;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Palabras clave")
	TSoftObjectPtr<UTexture2D> EnergyOnKillIcon;

	// Color de las palabras clave que no son ni buenas ni malas (empujar, robar...)
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Palabras clave")
	FLinearColor KeywordColor = FLinearColor(1.f, 0.78f, 0.35f, 1.f);

	// Tamaño del icono que va detrás de la palabra, respecto a la altura de la letra
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Palabras clave", meta = (ClampMin = "0.1"))
	float KeywordIconScale = 1.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Palabras clave", meta = (TitleProperty = "Name"))
	TArray<FCardKeyword> Keywords;

	const FCardKeyword* FindKeyword(FName Id) const;
};
