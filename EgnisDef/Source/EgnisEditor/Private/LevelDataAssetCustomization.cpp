#include "LevelDataAssetCustomization.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LevelDataAssetCustomization"

namespace
{
    constexpr float CellWidth = 60.f;
    constexpr float CellHeight = 60.f;

    ETileType GetTile(const ULevelDataAsset* Asset, int32 X, int32 Y)
    {
        if (Asset && Asset->GridRows.IsValidIndex(Y) && Asset->GridRows[Y].Columns.IsValidIndex(X))
        {
            return Asset->GridRows[Y].Columns[X];
        }
        return ETileType::Void;
    }

    FText GetTileName(ETileType Type)
    {
        return StaticEnum<ETileType>()->GetDisplayNameTextByValue(static_cast<int64>(Type));
    }

    FLinearColor GetTileColor(ETileType Type)
    {
        switch (Type)
        {
        case ETileType::Void:              return FLinearColor(0.02f, 0.02f, 0.02f);
        case ETileType::Normal:            return FLinearColor(0.20f, 0.45f, 0.18f);
        case ETileType::Obstacle:          return FLinearColor(0.30f, 0.30f, 0.30f);
        case ETileType::DamagingObstacle:  return FLinearColor(0.65f, 0.10f, 0.10f);
        case ETileType::BreakableObstacle: return FLinearColor(0.45f, 0.28f, 0.12f);
        case ETileType::ExplosiveObstacle: return FLinearColor(0.90f, 0.45f, 0.05f);
        case ETileType::Water:             return FLinearColor(0.08f, 0.25f, 0.70f);
        case ETileType::WaterPlatform:     return FLinearColor(0.25f, 0.55f, 0.70f);
        case ETileType::Ice:               return FLinearColor(0.75f, 0.92f, 1.00f);
        case ETileType::Fog:               return FLinearColor(0.55f, 0.50f, 0.65f);
        default:                           return FLinearColor::White;
        }
    }

    void CycleTile(ULevelDataAsset* Asset, int32 X, int32 Y, int32 Step)
    {
        if (!Asset)
        {
            return;
        }

        const FScopedTransaction Transaction(LOCTEXT("ChangeTileType", "Cambiar tipo de casilla"));
        Asset->Modify();
        Asset->ResizeGrid();

        if (!Asset->GridRows.IsValidIndex(Y) || !Asset->GridRows[Y].Columns.IsValidIndex(X))
        {
            return;
        }

        const UEnum* TileEnum = StaticEnum<ETileType>();
        const int32 NumTypes = TileEnum->NumEnums() - 1; // el último es ETileType_MAX

        ETileType& Tile = Asset->GridRows[Y].Columns[X];
        const int32 CurrentIndex = TileEnum->GetIndexByValue(static_cast<int64>(Tile));
        const int32 NextIndex = (CurrentIndex + Step + NumTypes) % NumTypes;
        Tile = static_cast<ETileType>(TileEnum->GetValueByIndex(NextIndex));
    }
}

TSharedRef<IDetailCustomization> FLevelDataAssetCustomization::MakeInstance()
{
    return MakeShared<FLevelDataAssetCustomization>();
}

void FLevelDataAssetCustomization::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
    CachedDetailBuilder = DetailBuilder;
    CustomizeDetails(*DetailBuilder);
}

void FLevelDataAssetCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);
    if (Objects.Num() != 1) return;

    TWeakObjectPtr<ULevelDataAsset> WeakAsset = Cast<ULevelDataAsset>(Objects[0].Get());
    const ULevelDataAsset* LevelAsset = WeakAsset.Get();
    if (!LevelAsset) return;
    
    const FSimpleDelegate RefreshGrid = FSimpleDelegate::CreateLambda([WeakBuilder = CachedDetailBuilder]()
    {
        if (TSharedPtr<IDetailLayoutBuilder> Builder = WeakBuilder.Pin())
        {
            Builder->ForceRefreshDetails();
        }
    });
    DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULevelDataAsset, Width))->SetOnPropertyValueChanged(RefreshGrid);
    DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULevelDataAsset, Height))->SetOnPropertyValueChanged(RefreshGrid);


    DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(ULevelDataAsset, GridRows));


    IDetailCategoryBuilder& GridCategory = DetailBuilder.EditCategory("VisualGrid", FText::FromString("Editor de casillas del Nivel"));
    
    TSharedRef<SGridPanel> GridPanel = SNew(SGridPanel);
    
    for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
    {
        for (int32 X = 0; X < LevelAsset->Width; ++X)
        {
            GridPanel->AddSlot(X, Y)
            .Padding(1.0f)
            [
                SNew(SBox)
                .WidthOverride(CellWidth)
                .HeightOverride(CellHeight)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .BorderBackgroundColor_Lambda([WeakAsset, X, Y]()
                    {
                        return FSlateColor(GetTileColor(GetTile(WeakAsset.Get(), X, Y)));
                    })
                    .HAlign(HAlign_Center)
                    .VAlign(VAlign_Center)
                    .ToolTipText_Lambda([WeakAsset, X, Y]()
                    {
                        return FText::Format(LOCTEXT("TileTooltip", "({0}, {1}) {2}"), X, Y, GetTileName(GetTile(WeakAsset.Get(), X, Y)));
                    })
                    .OnMouseButtonDown_Lambda([WeakAsset, X, Y](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
                    {
                        if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
                        {
                            CycleTile(WeakAsset.Get(), X, Y, 1);
                            return FReply::Handled();
                        }
                        if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
                        {
                            CycleTile(WeakAsset.Get(), X, Y, -1);
                            return FReply::Handled();
                        }
                        return FReply::Unhandled();
                    })
                    .OnMouseButtonUp_Lambda([](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
                    {
                        return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton ? FReply::Handled() : FReply::Unhandled();
                    })
                    [
                        SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
                        .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                        .Text_Lambda([WeakAsset, X, Y]()
                        {
                            return GetTileName(GetTile(WeakAsset.Get(), X, Y));
                        })
                        .ColorAndOpacity_Lambda([WeakAsset, X, Y]()
                        {
                            const bool bLightTile = GetTileColor(GetTile(WeakAsset.Get(), X, Y)).GetLuminance() > 0.5f;
                            return FSlateColor(bLightTile ? FLinearColor::Black : FLinearColor::White);
                        })
                    ]
                ]
            ];
        }
    }

    GridCategory.AddCustomRow(FText::FromString("GridMap"))
    .WholeRowContent()
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, 4.f)
        [
            SNew(STextBlock)
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .Text(LOCTEXT("GridHint", "Clic izquierdo: siguiente tipo · Clic derecho: tipo anterior"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Left)
        [
            GridPanel
        ]
    ];
}

#undef LOCTEXT_NAMESPACE
