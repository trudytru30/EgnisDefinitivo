#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Cards/CardDisplayTypes.h"
#include "CardEffectsPanelWidget.generated.h"

class UBaseCard;
class ACharacterBase;
class UCardEffectIconWidget;
class UPanelWidget;
class UImage;
class UTextBlock;

// Lista de efectos de una carta (un icono por efecto, en EffectsBox) y el icono de su tipo de lanzamiento.
// La carta llama a SetCard; los números se calculan con el personaje seleccionado y se actualizan solos
// cuando cambia el personaje o sus estadísticas
UCLASS(Abstract)
class EGNISDEF_API UCardEffectsPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Card UI")
	void SetCard(UBaseCard* InCard);

	UFUNCTION(BlueprintPure, Category = "Card UI")
	UBaseCard* GetCard() const { return Card; }

	UFUNCTION(BlueprintCallable, Category = "Card UI")
	void Refresh();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI")
	TSubclassOf<UCardEffectIconWidget> EffectIconClass;

	// Separación entre iconos (vale tanto si EffectsBox es vertical como horizontal)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI", meta = (ClampMin = "0"))
	float IconSpacing = 4.f;

	// Cada cuánto se mira si ha cambiado el personaje seleccionado o sus estadísticas
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI", meta = (ClampMin = "0.05"))
	float CheckInterval = 0.25f;

	// En el diseñador enseña dos efectos de ejemplo para poder maquetar
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card UI")
	bool bPreviewInDesigner = true;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI", meta = (BindWidget))
	TObjectPtr<UPanelWidget> EffectsBox;

	UPROPERTY(BlueprintReadOnly, Category = "Card UI", meta = (BindWidgetOptional))
	TObjectPtr<UImage> LaunchTypeIcon;

	// Se ve en lugar del icono de lanzamiento mientras no haya uno en Project Settings
	UPROPERTY(BlueprintReadOnly, Category = "Card UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LaunchTypeText;

private:
	const ACharacterBase* FindCaster() const;
	bool HasCasterChanged();
	void ShowDisplays(const TArray<FCardEffectDisplay>& Displays);
	void ShowLaunchType(ECardLaunchType LaunchType, bool bVisible);

	UPROPERTY(Transient)
	TObjectPtr<UBaseCard> Card;

	TArray<FCardEffectDisplay> Shown;
	TWeakObjectPtr<const ACharacterBase> LastCaster;
	float LastPower = -1.f;
	float LastPiety = -1.f;
	FGameplayTagContainer LastTags;
	float TimeSinceCheck = 0.f;
};
