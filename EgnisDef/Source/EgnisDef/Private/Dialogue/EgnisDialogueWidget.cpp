#include "Dialogue/EgnisDialogueWidget.h"

#include "Dialogue/EgnisDialogueComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"

void UEgnisDialogueChoiceProxy::HandleClicked()
{
	if (UEgnisDialogueWidget* Widget = Owner.Get())
	{
		Widget->RequestChoice(Index);
	}
}

UEgnisDialogueWidget::UEgnisDialogueWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UEgnisDialogueWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildDefaultTree();
}

TSharedRef<SWidget> UEgnisDialogueWidget::RebuildWidget()
{
	BuildDefaultTree();
	return Super::RebuildWidget();
}

void UEgnisDialogueWidget::BuildDefaultTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return; // ya existe (o es un Widget Blueprint con su propio diseño)
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.88f));
	Panel->SetPadding(FMargin(36.f, 24.f));
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Fill);
		PanelSlot->SetVerticalAlignment(VAlign_Bottom);
		PanelSlot->SetPadding(FMargin(120.f, 0.f, 120.f, 60.f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
	Panel->SetContent(Box);

	SpeakerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeakerText"));
	{
		FSlateFontInfo Font = SpeakerText->GetFont();
		Font.Size = 22;
		Font.TypefaceFontName = TEXT("Bold");
		SpeakerText->SetFont(Font);
		SpeakerText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.70f, 0.30f)));
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(SpeakerText))
		{
			S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}
	}

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BodyText"));
	{
		FSlateFontInfo Font = BodyText->GetFont();
		Font.Size = 20;
		BodyText->SetFont(Font);
		BodyText->SetAutoWrapText(true);
		BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f)));
		Box->AddChildToVerticalBox(BodyText);
	}

	UVerticalBox* Choices = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChoicesBox"));
	ChoicesBox = Choices;
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Choices))
	{
		S->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}

	ContinueHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContinueHint"));
	{
		FSlateFontInfo Font = ContinueHint->GetFont();
		Font.Size = 13;
		ContinueHint->SetFont(Font);
		ContinueHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(ContinueHint))
		{
			S->SetHorizontalAlignment(HAlign_Right);
			S->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		}
	}
}

void UEgnisDialogueWidget::ShowLine(const FEgnisDialogueLine& Line)
{
	BuildDefaultTree();

	if (SpeakerText)
	{
		SpeakerText->SetText(Line.Speaker);
		SpeakerText->SetVisibility(Line.Speaker.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (BodyText)
	{
		BodyText->SetText(Line.Text);
	}

	NumChoices = Line.Choices.Num();
	bIsLast = Line.bIsLast;

	if (ChoicesBox)
	{
		ChoicesBox->ClearChildren();
		ChoiceProxies.Reset();

		for (int32 i = 0; i < Line.Choices.Num(); ++i)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			Button->SetBackgroundColor(FLinearColor(0.25f, 0.18f, 0.12f, 1.f));

			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = 18;
			Label->SetFont(Font);
			Label->SetAutoWrapText(true);
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.92f, 0.8f)));
			Label->SetText(FText::Format(FText::FromString(TEXT("{0}. {1}")), FText::AsNumber(i + 1), Line.Choices[i].Text));
			Button->SetContent(Label);

			UEgnisDialogueChoiceProxy* Proxy = NewObject<UEgnisDialogueChoiceProxy>(this);
			Proxy->Owner = this;
			Proxy->Index = i;
			ChoiceProxies.Add(Proxy);
			Button->OnClicked.AddDynamic(Proxy, &UEgnisDialogueChoiceProxy::HandleClicked);

			UPanelSlot* PanelSlot = ChoicesBox->AddChild(Button);
			if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(PanelSlot))
			{
				VSlot->SetPadding(FMargin(0.f, 4.f));
				VSlot->SetHorizontalAlignment(HAlign_Fill);
			}
		}
		ChoicesBox->SetVisibility(NumChoices > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (ContinueHint)
	{
		FString Hint;
		if (NumChoices > 0)
		{
			Hint = FString::Printf(TEXT("Elige una opcion (clic o teclas 1-%d)"), NumChoices);
		}
		else if (bIsLast)
		{
			Hint = TEXT("Clic / Espacio para cerrar");
		}
		else
		{
			Hint = TEXT("Clic / Espacio para continuar");
		}
		ContinueHint->SetText(FText::FromString(Hint));
	}

	LastShowTime = FPlatformTime::Seconds();
	SetKeyboardFocus();
	OnLineDisplayed(Line);
}

void UEgnisDialogueWidget::RequestContinue()
{
	if (FPlatformTime::Seconds() - LastShowTime < 0.15)
	{
		return; // evita saltarse líneas con un doble clic
	}
	if (NumChoices > 0)
	{
		return;
	}
	if (UEgnisDialogueComponent* Comp = DialogueComponent.Get())
	{
		Comp->Advance();
	}
}

void UEgnisDialogueWidget::RequestChoice(int32 Index)
{
	if (FPlatformTime::Seconds() - LastShowTime < 0.15)
	{
		return;
	}
	if (Index < 0 || Index >= NumChoices)
	{
		return;
	}
	if (UEgnisDialogueComponent* Comp = DialogueComponent.Get())
	{
		Comp->SelectChoice(Index);
	}
}

FReply UEgnisDialogueWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::SpaceBar || Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		RequestContinue();
		return FReply::Handled();
	}

	static const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
		EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	static const FKey NumPadKeys[] = { EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour,
		EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine };
	for (int32 i = 0; i < 9; ++i)
	{
		if (Key == NumberKeys[i] || Key == NumPadKeys[i])
		{
			RequestChoice(i);
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UEgnisDialogueWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		RequestContinue();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
