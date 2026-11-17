#include "Cards/CardEffectsPanelWidget.h"
#include "Cards/BaseCard.h"
#include "Cards/CardDisplayLibrary.h"
#include "Cards/CardDisplaySettings.h"
#include "Cards/CardEffectIconWidget.h"
#include "Characters/Ally.h"
#include "Characters/BoardPlayerController.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "CardDisplay"

void UCardEffectsPanelWidget::SetCard(UBaseCard* InCard)
{
	Card = InCard;
	Shown.Reset();
	HasCasterChanged();
	Refresh();
}

void UCardEffectsPanelWidget::Refresh()
{
	if (!Card)
	{
		ShowDisplays({});
		ShowLaunchType(ECardLaunchType::None, false);
		return;
	}
	ShowDisplays(UCardDisplayLibrary::GetEffectDisplays(Card, FindCaster()));
	ShowLaunchType(UCardDisplayLibrary::GetLaunchType(Card), true);
}

void UCardEffectsPanelWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!IsDesignTime() || !bPreviewInDesigner)
	{
		return;
	}

	const UEgnisCardDisplaySettings* Settings = GetDefault<UEgnisCardDisplaySettings>();
	FCardEffectDisplay Damage;
	Damage.Label = LOCTEXT("PreviewDamage", "Daño");
	Damage.Color = Settings->GoodColor;
	Damage.bHasValue = true;
	Damage.Value = 12;

	FCardEffectDisplay Fatigue;
	Fatigue.Label = LOCTEXT("PreviewFatigue", "Fatiga");
	Fatigue.bGoodForPlayer = false;
	Fatigue.Color = Settings->BadColor;
	Fatigue.bHasValue = true;
	Fatigue.Value = 2;

	Shown.Reset();
	ShowDisplays({ Damage, Fatigue });
	ShowLaunchType(ECardLaunchType::Enemy, true);
}

void UCardEffectsPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Va encima de la carta: no puede quitarle el ratón (pasar por encima, arrastrar, clic)
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCardEffectsPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!Card)
	{
		return;
	}
	TimeSinceCheck += InDeltaTime;
	if (TimeSinceCheck < CheckInterval)
	{
		return;
	}
	TimeSinceCheck = 0.f;
	if (HasCasterChanged())
	{
		Refresh();
	}
}

const ACharacterBase* UCardEffectsPanelWidget::FindCaster() const
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController && GetWorld())
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}
	const ABoardPlayerController* BoardController = Cast<ABoardPlayerController>(PlayerController);
	return BoardController ? BoardController->GetSelectedAlly() : nullptr;
}

// Solo se recalculan los números cuando cambia algo de lo que dependen: así no se llama a la fórmula
// de daño (que escribe en el log) cuatro veces por segundo
bool UCardEffectsPanelWidget::HasCasterChanged()
{
	const ACharacterBase* Caster = FindCaster();
	float Power = 0.f;
	float Piety = 0.f;
	FGameplayTagContainer Tags;
	if (const AAlly* Ally = Cast<AAlly>(Caster))
	{
		Power = Ally->GetCharacterPower();
		Piety = Ally->GetCharacterPiety();
	}
	if (Caster)
	{
		if (const UAbilitySystemComponent* ASC = Caster->GetAbilitySystemComponent())
		{
			ASC->GetOwnedGameplayTags(Tags);
		}
	}

	const bool bChanged = Caster != LastCaster.Get() || Power != LastPower || Piety != LastPiety || Tags != LastTags;
	LastCaster = Caster;
	LastPower = Power;
	LastPiety = Piety;
	LastTags = MoveTemp(Tags);
	return bChanged;
}

void UCardEffectsPanelWidget::ShowDisplays(const TArray<FCardEffectDisplay>& Displays)
{
	if (!EffectsBox)
	{
		return;
	}
	if (Displays == Shown && EffectsBox->GetChildrenCount() == Displays.Num())
	{
		return;
	}
	Shown = Displays;

	if (EffectsBox->GetChildrenCount() != Displays.Num())
	{
		EffectsBox->ClearChildren();
		if (!EffectIconClass)
		{
			return;
		}
		for (int32 Index = 0; Index < Displays.Num(); ++Index)
		{
			UCardEffectIconWidget* IconWidget = CreateWidget<UCardEffectIconWidget>(this, EffectIconClass);
			if (!IconWidget)
			{
				continue;
			}
			IconWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			const bool bLast = Index == Displays.Num() - 1;
			UPanelSlot* IconSlot = EffectsBox->AddChild(IconWidget);
			if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(IconSlot))
			{
				VerticalSlot->SetPadding(FMargin(0.f, 0.f, 0.f, bLast ? 0.f : IconSpacing));
			}
			else if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(IconSlot))
			{
				HorizontalSlot->SetPadding(FMargin(0.f, 0.f, bLast ? 0.f : IconSpacing, 0.f));
			}
		}
	}

	for (int32 Index = 0; Index < Displays.Num(); ++Index)
	{
		if (UCardEffectIconWidget* IconWidget = Cast<UCardEffectIconWidget>(EffectsBox->GetChildAt(Index)))
		{
			IconWidget->SetDisplay(Displays[Index]);
		}
	}
}

void UCardEffectsPanelWidget::ShowLaunchType(ECardLaunchType LaunchType, bool bVisible)
{
	UTexture2D* Texture = bVisible ? UCardDisplayLibrary::GetLaunchTypeIcon(LaunchType) : nullptr;

	if (LaunchTypeIcon)
	{
		if (Texture)
		{
			LaunchTypeIcon->SetBrushFromTexture(Texture, false);
		}
		LaunchTypeIcon->SetColorAndOpacity(GetDefault<UEgnisCardDisplaySettings>()->LaunchTypeColor);
		const bool bShowImage = bVisible && (Texture || !LaunchTypeText);
		LaunchTypeIcon->SetVisibility(bShowImage ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (LaunchTypeText)
	{
		LaunchTypeText->SetText(UCardDisplayLibrary::GetLaunchTypeName(LaunchType));
		LaunchTypeText->SetVisibility(bVisible && !Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE
