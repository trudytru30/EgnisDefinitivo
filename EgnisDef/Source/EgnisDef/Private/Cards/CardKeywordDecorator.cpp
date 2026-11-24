#include "Cards/CardKeywordDecorator.h"
#include "Cards/CardDisplayLibrary.h"
#include "Cards/CardDisplaySettings.h"
#include "Components/RichTextBlock.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/ITextDecorator.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	const TCHAR* KeywordTag = TEXT("kw");
	const TCHAR* IconTag = TEXT("kwicon");

	class FCardKeywordRichTextDecorator : public FRichTextDecorator
	{
	public:
		FCardKeywordRichTextDecorator(URichTextBlock* InOwner, UCardKeywordDecorator* InDecorator)
			: FRichTextDecorator(InOwner)
			, Decorator(InDecorator)
		{
		}

		virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
		{
			return (RunParseResult.Name == KeywordTag || RunParseResult.Name == IconTag) && RunParseResult.MetaData.Contains(TEXT("id"));
		}

	protected:
		virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& TextStyle) const override
		{
			if (RunInfo.Name != IconTag || !Decorator.IsValid())
			{
				return nullptr;
			}

			const FName Id(*RunInfo.MetaData[TEXT("id")]);
			const FSlateBrush* Brush = Decorator->FindIconBrush(Id);
			if (!Brush)
			{
				return nullptr;
			}

			float Size = TextStyle.Font.Size * 1.33f;
			if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
			{
				Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->GetMaxCharacterHeight(TextStyle.Font, 1.f);
			}
			Size *= GetDefault<UEgnisCardDisplaySettings>()->KeywordIconScale;

			return SNew(SBox)
				.Padding(FMargin(Size * 0.15f, 0.f, 0.f, 0.f))
				.HeightOverride(Size)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(Size)
					.HeightOverride(Size)
					[
						SNew(SImage)
						.Image(Brush)
						.ColorAndOpacity(FSlateColor(UCardDisplayLibrary::GetKeywordColor(Id)))
					]
				];
		}

		virtual void CreateDecoratorText(const FTextRunInfo& RunInfo, FTextBlockStyle& InOutTextStyle, FString& InOutString) const override
		{
			if (RunInfo.Name == KeywordTag)
			{
				InOutTextStyle.SetColorAndOpacity(FSlateColor(UCardDisplayLibrary::GetKeywordColor(FName(*RunInfo.MetaData[TEXT("id")]))));
				InOutString += RunInfo.Content.ToString();
			}
		}

	private:
		TWeakObjectPtr<UCardKeywordDecorator> Decorator;
	};
}

UCardKeywordDecorator::UCardKeywordDecorator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<ITextDecorator> UCardKeywordDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShared<FCardKeywordRichTextDecorator>(InOwner, this);
}

const FSlateBrush* UCardKeywordDecorator::FindIconBrush(FName Id)
{
	if (const TSharedRef<FSlateBrush>* Existing = Brushes.Find(Id))
	{
		return &Existing->Get();
	}

	FCardKeywordInfo Info;
	if (!UCardDisplayLibrary::GetKeywordInfo(Id, Info) || !Info.Icon)
	{
		return nullptr;
	}

	Icons.Add(Id, Info.Icon);
	const TSharedRef<FSlateBrush> Brush = MakeShared<FSlateBrush>();
	Brush->SetResourceObject(Info.Icon);
	Brush->ImageSize = FVector2D(Info.Icon->GetSizeX(), Info.Icon->GetSizeY());
	Brush->DrawAs = ESlateBrushDrawType::Image;
	Brushes.Add(Id, Brush);
	return &Brush.Get();
}
