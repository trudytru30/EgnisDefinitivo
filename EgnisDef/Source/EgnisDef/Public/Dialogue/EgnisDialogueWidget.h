#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dialogue/EgnisDialogueTypes.h"
#include "EgnisDialogueWidget.generated.h"

class UTextBlock;
class UPanelWidget;
class UButton;
class UEgnisDialogueComponent;
class UEgnisDialogueWidget;

/** Pequeño helper para saber qué botón de opción se ha pulsado. */
UCLASS()
class EGNISDEF_API UEgnisDialogueChoiceProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TWeakObjectPtr<UEgnisDialogueWidget> Owner;

	int32 Index = 0;

	UFUNCTION()
	void HandleClicked();
};

/**
 * Caja de diálogo. Funciona tal cual (se construye por código), pero también puede
 * usarse como clase padre de un Widget Blueprint: si el WBP tiene widgets llamados
 * SpeakerText, BodyText, ChoicesBox y ContinueHint, se usan en vez de los de por defecto.
 * Controles: clic o Espacio/Enter para continuar, 1-9 o clic para elegir opción.
 */
UCLASS()
class EGNISDEF_API UEgnisDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UEgnisDialogueWidget(const FObjectInitializer& ObjectInitializer);

	void SetDialogueComponent(UEgnisDialogueComponent* InComponent);

	/** Muestra una línea (con o sin opciones). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ShowLine(const FEgnisDialogueLine& Line);

	/** Para que un WBP hijo pueda añadir animaciones, sonidos, etc. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void OnLineDisplayed(const FEgnisDialogueLine& Line);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void RequestContinue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void RequestChoice(int32 Index);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue")
	TObjectPtr<UTextBlock> SpeakerText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue")
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue")
	TObjectPtr<UPanelWidget> ChoicesBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dialogue")
	TObjectPtr<UTextBlock> ContinueHint;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void BuildDefaultTree();

	UPROPERTY(Transient)
	TWeakObjectPtr<UEgnisDialogueComponent> DialogueComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEgnisDialogueChoiceProxy>> ChoiceProxies;

	int32 NumChoices = 0;
	bool bIsLast = false;
	double LastShowTime = 0.0;
};
