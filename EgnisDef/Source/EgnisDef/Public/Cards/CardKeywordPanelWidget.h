#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "CardKeywordPanelWidget.generated.h"

class UBaseCard;
class UVerticalBox;

// Panel para poner a la derecha de la carta en el inspector: un recuadro con la descripción (palabras clave de color
// y con su icono) y un recuadro por cada efecto de la carta explicándolo. Los recuadros salen solos de los efectos
// (UCardDisplayLibrary::GetCardKeywords) y los textos de Project Settings > Egnis > Iconos de cartas > Palabras clave.
// Se usa tal cual, sin WBP hijo: se coloca en el inspector y se le llama a SetCard
UCLASS()
class EGNISDEF_API UCardKeywordPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCardKeywordPanelWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Card UI")
	void SetCard(UBaseCard* InCard);

	UFUNCTION(BlueprintPure, Category = "Card UI")
	UBaseCard* GetCard() const { return Card; }

	UFUNCTION(BlueprintCallable, Category = "Card UI")
	void Refresh();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI")
	bool bShowDescription = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI", meta = (ClampMin = "50"))
	float PanelWidth = 340.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI", meta = (ClampMin = "0"))
	float BoxSpacing = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI")
	FMargin BoxPadding = FMargin(12.f, 8.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI")
	FSlateBrush BoxBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI", meta = (ClampMin = "0"))
	float IconSize = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI|Text")
	FTextBlockStyle DescriptionStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI|Text")
	FSlateFontInfo NameFont;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI|Text")
	FSlateFontInfo BodyFont;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI|Text")
	FSlateColor BodyColor = FSlateColor(FLinearColor::White);

	// Carta que se enseña en el diseñador para maquetar. Sin ella sale un ejemplo
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI")
	TObjectPtr<UBaseCard> PreviewCard;

private:
	void BuildTree();
	void AddBox(UWidget* Content);
	void AddDescriptionBox(const FText& RichText);
	void AddKeywordBox(const struct FCardKeywordInfo& Info);

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> BoxesPanel;

	UPROPERTY(Transient)
	TObjectPtr<UBaseCard> Card;
};
