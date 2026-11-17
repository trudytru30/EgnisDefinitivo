#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Cards/CardDisplayTypes.h"
#include "CardDisplayLibrary.generated.h"

class UBaseCard;
class UCardEffect;
class ACharacterBase;
class UTexture2D;

// Lo que enseña una carta de sus efectos. Caster es quien la lanzaría (el personaje seleccionado):
// sin él, los números que dependen del personaje (daño, tiempo...) no se muestran
UCLASS()
class EGNISDEF_API UCardDisplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Card UI")
	static TArray<FCardEffectDisplay> GetEffectDisplays(const UBaseCard* Card, const ACharacterBase* Caster);

	UFUNCTION(BlueprintPure, Category = "Card UI")
	static FCardEffectDisplay GetEffectDisplay(const UCardEffect* Effect, const ACharacterBase* Caster);

	UFUNCTION(BlueprintPure, Category = "Card UI")
	static bool IsEffectGoodForPlayer(const UCardEffect* Effect);

	UFUNCTION(BlueprintPure, Category = "Card UI")
	static ECardLaunchType GetLaunchType(const UBaseCard* Card);

	UFUNCTION(BlueprintPure, Category = "Card UI")
	static UTexture2D* GetLaunchTypeIcon(ECardLaunchType LaunchType);

	UFUNCTION(BlueprintPure, Category = "Card UI")
	static FText GetLaunchTypeName(ECardLaunchType LaunchType);
};
