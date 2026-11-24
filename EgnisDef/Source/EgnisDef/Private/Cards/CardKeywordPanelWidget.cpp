#include "Cards/CardKeywordPanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Cards/BaseCard.h"
#include "Cards/CardDisplayLibrary.h"
#include "Cards/CardKeywordDecorator.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "CardKeywordPanel"

UCardKeywordPanelWidget::UCardKeywordPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BoxBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	BoxBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.85f));
	BoxBrush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(6.f, 6.f, 6.f, 6.f), FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.15f)), 1.f);

	DescriptionStyle.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
	DescriptionStyle.SetColorAndOpacity(FSlateColor(FLinearColor::White));
	NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 15);
	BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 13);
}

TSharedRef<SWidget> UCardKeywordPanelWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
		Refresh();
	}
	return Super::RebuildWidget();
}

void UCardKeywordPanelWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		Card = PreviewCard;
		Refresh();
	}
}

void UCardKeywordPanelWidget::BuildTree()
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	Size->SetWidthOverride(PanelWidth);
	WidgetTree->RootWidget = Size;

	BoxesPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Boxes"));
	Size->AddChild(BoxesPanel);
}

void UCardKeywordPanelWidget::SetCard(UBaseCard* InCard)
{
	Card = InCard;
	Refresh();
}

void UCardKeywordPanelWidget::Refresh()
{
	if (!BoxesPanel)
	{
		return;
	}
	BoxesPanel->ClearChildren();

	if (!IsDesignTime())
	{
		SetVisibility(Card ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Card)
	{
		if (bShowDescription && !Card->GetDescription().IsEmpty())
		{
			AddDescriptionBox(UCardDisplayLibrary::GetRichDescription(Card));
		}
		for (const FCardKeywordInfo& Info : UCardDisplayLibrary::GetCardKeywords(Card))
		{
			AddKeywordBox(Info);
		}
		return;
	}

	if (IsDesignTime())
	{
		const FText Example = LOCTEXT("Example", "Hace daño medio y aplica quemadura. Después empuja al objetivo");
		if (bShowDescription)
		{
			AddDescriptionBox(UCardDisplayLibrary::MarkupKeywords(Example));
		}
		for (const FName Id : { FName(TEXT("Damage")), FName(TEXT("Burn")), FName(TEXT("Push")) })
		{
			FCardKeywordInfo Info;
			if (UCardDisplayLibrary::GetKeywordInfo(Id, Info))
			{
				AddKeywordBox(Info);
			}
		}
	}
}

void UCardKeywordPanelWidget::AddBox(UWidget* Content)
{
	UBorder* Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Box->SetBrush(BoxBrush);
	Box->SetPadding(BoxPadding);
	Box->SetContent(Content);

	UVerticalBoxSlot* BoxSlot = BoxesPanel->AddChildToVerticalBox(Box);
	BoxSlot->SetPadding(FMargin(0.f, BoxesPanel->GetChildrenCount() > 1 ? BoxSpacing : 0.f, 0.f, 0.f));
}

void UCardKeywordPanelWidget::AddDescriptionBox(const FText& RichText)
{
	URichTextBlock* Description = WidgetTree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass());
	Description->SetDecorators({ UCardKeywordDecorator::StaticClass() });
	Description->SetDefaultTextStyle(DescriptionStyle);
	Description->SetAutoWrapText(true);
	Description->SetText(RichText);
	AddBox(Description);
}

void UCardKeywordPanelWidget::AddKeywordBox(const FCardKeywordInfo& Info)
{
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	if (Info.Icon)
	{
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Icon->SetBrushFromTexture(Info.Icon);
		Icon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
		Icon->SetColorAndOpacity(Info.Color);
		UHorizontalBoxSlot* IconSlot = Header->AddChildToHorizontalBox(Icon);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	}

	UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Name->SetText(Info.Name);
	Name->SetFont(NameFont);
	Name->SetColorAndOpacity(FSlateColor(Info.Color));
	Header->AddChildToHorizontalBox(Name)->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(Header);

	if (!Info.Description.IsEmpty())
	{
		UTextBlock* Body = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Body->SetText(Info.Description);
		Body->SetFont(BodyFont);
		Body->SetColorAndOpacity(BodyColor);
		Body->SetAutoWrapText(true);
		Content->AddChildToVerticalBox(Body)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	AddBox(Content);
}

#undef LOCTEXT_NAMESPACE
