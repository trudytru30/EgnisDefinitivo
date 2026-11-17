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
};
