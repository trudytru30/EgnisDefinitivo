#include "Cards/CardEffectIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UCardEffectIconWidget::SetDisplay(const FCardEffectDisplay& InDisplay)
{
	Display = InDisplay;
	const bool bHasIcon = Display.Icon != nullptr;

	if (Icon)
	{
		if (bHasIcon)
		{
			Icon->SetBrushFromTexture(Display.Icon, false);
		}
		Icon->SetColorAndOpacity(Display.Color);
		Icon->SetVisibility(bHasIcon || !LabelText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (LabelText)
	{
		LabelText->SetText(Display.Label);
		LabelText->SetColorAndOpacity(FSlateColor(Display.Color));
		LabelText->SetVisibility(bHasIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (ValueText)
	{
		ValueText->SetText(FText::AsNumber(Display.Value));
		ValueText->SetVisibility(Display.bHasValue ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetToolTipText(Display.Label);
	OnDisplayChanged(Display);
}
