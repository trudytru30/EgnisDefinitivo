#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Cards/CardDisplayTypes.h"
#include "CardEffectIconWidget.generated.h"

class UImage;
class UTextBlock;

// Un efecto de la carta: su icono teñido de bueno/malo y su número al lado.
// Mientras el efecto no tenga icono se ve LabelText (el nombre del efecto) en su lugar
UCLASS(Abstract)
class EGNISDEF_API UCardEffectIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Card UI")
	void SetDisplay(const FCardEffectDisplay& InDisplay);

	UFUNCTION(BlueprintPure, Category = "Card UI")
	FCardEffectDisplay GetDisplay() const { return Display; }

protected:
	// Para añadir cosas en el BP (animaciones, tooltip propio...) cada vez que cambian los datos
	UFUNCTION(BlueprintImplementableEvent, Category = "Card UI")
	void OnDisplayChanged(const FCardEffectDisplay& NewDisplay);

	UPROPERTY(BlueprintReadOnly, Category = "Card UI", meta = (BindWidget))
	TObjectPtr<UImage> Icon;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

private:
	UPROPERTY(Transient)
	FCardEffectDisplay Display;
};
