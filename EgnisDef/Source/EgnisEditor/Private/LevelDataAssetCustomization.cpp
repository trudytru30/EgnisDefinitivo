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
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LevelDataAssetCustomization"

namespace
{
    constexpr float CellWidth = 60.f;
    constexpr float CellHeight = 60.f;

    enum class EGridEditMode : uint8
    {
        TileTypes,
        DeployZones
    };

    // Se guardan fuera de la customización porque ésta se vuelve a crear cada vez que se refresca el panel
    EGridEditMode GEditMode = EGridEditMode::TileTypes;
    int32 GActiveZone = 0;

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

    // El color de cada zona sale de su índice: no hace falta guardarlo en el asset
    FLinearColor GetZoneColor(int32 ZoneIndex)
    {
        static const FLinearColor Palette[] = {
            FLinearColor(1.00f, 0.85f, 0.10f), // amarillo
            FLinearColor(0.10f, 0.85f, 1.00f), // cian
            FLinearColor(1.00f, 0.30f, 0.80f), // magenta
            FLinearColor(1.00f, 0.55f, 0.10f), // naranja
            FLinearColor(0.60f, 1.00f, 0.30f), // lima
            FLinearColor(0.70f, 0.50f, 1.00f), // lila
        };
        return Palette[ZoneIndex % UE_ARRAY_COUNT(Palette)];
    }

    int32 GetZoneIndex(const ULevelDataAsset* Asset, int32 X, int32 Y)
    {
        return Asset ? Asset->FindDeployZoneIndex(FTileCoord(X, Y)) : INDEX_NONE;
    }

    FText GetZoneName(const ULevelDataAsset* Asset, int32 ZoneIndex)
    {
        if (!Asset || !Asset->DeployZones.IsValidIndex(ZoneIndex))
        {
            return FText::GetEmpty();
        }

        const FText& Name = Asset->DeployZones[ZoneIndex].Name;
        return Name.IsEmpty() ? FText::Format(LOCTEXT("ZoneDefaultName", "Zona {0}"), ZoneIndex + 1) : Name;
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

    // Mete la casilla en la zona (sacándola de la que estuviera) o la quita de todas si ZoneIndex es INDEX_NONE
    void SetTileZone(ULevelDataAsset* Asset, int32 X, int32 Y, int32 ZoneIndex)
    {
        if (!Asset || (ZoneIndex != INDEX_NONE && !Asset->DeployZones.IsValidIndex(ZoneIndex)))
        {
            return;
        }

        const FTileCoord Tile(X, Y);
        if (Asset->FindDeployZoneIndex(Tile) == ZoneIndex)
        {
            return;
        }

        const FScopedTransaction Transaction(LOCTEXT("ChangeTileZone", "Cambiar zona de casilla"));
        Asset->Modify();

        for (FDeployZone& Zone : Asset->DeployZones)
        {
            Zone.Tiles.Remove(Tile);
        }
        if (ZoneIndex != INDEX_NONE)
        {
            Asset->DeployZones[ZoneIndex].Tiles.Add(Tile);
        }
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
    // Al añadir o borrar zonas desde el array hay que rehacer la barra de zonas
    DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULevelDataAsset, DeployZones))->SetOnPropertyValueChanged(RefreshGrid);


    DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(ULevelDataAsset, GridRows));

    if (!LevelAsset->DeployZones.IsValidIndex(GActiveZone))
    {
        GActiveZone = FMath::Max(0, LevelAsset->DeployZones.Num() - 1);
    }


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
                    // Marco con el color de la zona de despliegue (transparente si la casilla no es de ninguna)
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .Padding(4.f)
                    .BorderBackgroundColor_Lambda([WeakAsset, X, Y]()
                    {
                        const int32 ZoneIndex = GetZoneIndex(WeakAsset.Get(), X, Y);
                        return FSlateColor(ZoneIndex == INDEX_NONE ? FLinearColor::Transparent : GetZoneColor(ZoneIndex));
                    })
                    .ToolTipText_Lambda([WeakAsset, X, Y]()
                    {
                        const ULevelDataAsset* Asset = WeakAsset.Get();
                        const int32 ZoneIndex = GetZoneIndex(Asset, X, Y);
                        const FText ZoneText = ZoneIndex == INDEX_NONE ? LOCTEXT("NoZone", "Sin zona de despliegue") : GetZoneName(Asset, ZoneIndex);
                        return FText::Format(LOCTEXT("TileTooltip", "({0}, {1}) {2}\n{3}"), X, Y, GetTileName(GetTile(Asset, X, Y)), ZoneText);
                    })
                    .OnMouseButtonDown_Lambda([WeakAsset, X, Y](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
                    {
                        const bool bLeft = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
                        const bool bRight = MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
                        if (!bLeft && !bRight)
                        {
                            return FReply::Unhandled();
                        }

                        if (GEditMode == EGridEditMode::TileTypes)
                        {
                            CycleTile(WeakAsset.Get(), X, Y, bLeft ? 1 : -1);
                        }
                        else
                        {
                            SetTileZone(WeakAsset.Get(), X, Y, bLeft ? GActiveZone : INDEX_NONE);
                        }
                        return FReply::Handled();
                    })
                    .OnMouseButtonUp_Lambda([](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
                    {
                        return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton ? FReply::Handled() : FReply::Unhandled();
                    })
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                        .BorderBackgroundColor_Lambda([WeakAsset, X, Y]()
                        {
                            return FSlateColor(GetTileColor(GetTile(WeakAsset.Get(), X, Y)));
                        })
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
                            .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                            .Text_Lambda([WeakAsset, X, Y]()
                            {
                                const ULevelDataAsset* Asset = WeakAsset.Get();
                                if (GEditMode == EGridEditMode::DeployZones)
                                {
                                    const int32 ZoneIndex = GetZoneIndex(Asset, X, Y);
                                    return ZoneIndex == INDEX_NONE ? FText::GetEmpty() : FText::Format(LOCTEXT("ZoneShort", "Z{0}"), ZoneIndex + 1);
                                }
                                return GetTileName(GetTile(Asset, X, Y));
                            })
                            .ColorAndOpacity_Lambda([WeakAsset, X, Y]()
                            {
                                const bool bLightTile = GetTileColor(GetTile(WeakAsset.Get(), X, Y)).GetLuminance() > 0.5f;
                                return FSlateColor(bLightTile ? FLinearColor::Black : FLinearColor::White);
                            })
                        ]
                    ]
                ]
            ];
        }
    }

    // Barra de zonas: elegir la zona activa y crear zonas nuevas (solo visible en modo zonas)
    TSharedRef<SWrapBox> ZoneBar = SNew(SWrapBox)
        .UseAllottedSize(true)
        .InnerSlotPadding(FVector2D(4.f, 4.f));

    for (int32 ZoneIndex = 0; ZoneIndex < LevelAsset->DeployZones.Num(); ++ZoneIndex)
    {
        ZoneBar->AddSlot()
        [
            // Marco blanco alrededor de la zona activa
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
            .Padding(2.f)
            .BorderBackgroundColor_Lambda([ZoneIndex]()
            {
                return FSlateColor(GActiveZone == ZoneIndex ? FLinearColor::White : FLinearColor::Transparent);
            })
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                .BorderBackgroundColor(GetZoneColor(ZoneIndex))
                .Padding(FMargin(8.f, 4.f))
                .OnMouseButtonDown_Lambda([ZoneIndex](const FGeometry&, const FPointerEvent&) -> FReply
                {
                    GActiveZone = ZoneIndex;
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock)
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                    .ColorAndOpacity(FLinearColor::Black)
                    .Text_Lambda([WeakAsset, ZoneIndex]()
                    {
                        const ULevelDataAsset* Asset = WeakAsset.Get();
                        if (!Asset || !Asset->DeployZones.IsValidIndex(ZoneIndex))
                        {
                            return FText::GetEmpty();
                        }

                        const FDeployZone& Zone = Asset->DeployZones[ZoneIndex];
                        return FText::Format(LOCTEXT("ZoneButton", "Z{0} {1} · {2} casillas · máx {3}"),
                            ZoneIndex + 1, GetZoneName(Asset, ZoneIndex), Zone.Tiles.Num(), Zone.MaxHeroes);
                    })
                ]
            ]
        ];
    }

    ZoneBar->AddSlot()
    [
        SNew(SButton)
        .Text(LOCTEXT("AddZone", "+ Zona"))
        .OnClicked_Lambda([WeakAsset, WeakBuilder = CachedDetailBuilder]()
        {
            if (ULevelDataAsset* Asset = WeakAsset.Get())
            {
                const FScopedTransaction Transaction(LOCTEXT("AddZoneTransaction", "Añadir zona de despliegue"));
                Asset->Modify();
                Asset->DeployZones.AddDefaulted();
                GActiveZone = Asset->DeployZones.Num() - 1;
            }

            if (TSharedPtr<IDetailLayoutBuilder> Builder = WeakBuilder.Pin())
            {
                Builder->ForceRefreshDetails();
            }
            return FReply::Handled();
        })
    ];

    GridCategory.AddCustomRow(FText::FromString("GridMap"))
    .WholeRowContent()
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Left)
        .Padding(0.f, 4.f)
        [
            SNew(SSegmentedControl<EGridEditMode>)
            .Value_Lambda([]() { return GEditMode; })
            .OnValueChanged_Lambda([](EGridEditMode NewMode) { GEditMode = NewMode; })
            + SSegmentedControl<EGridEditMode>::Slot(EGridEditMode::TileTypes)
            .Text(LOCTEXT("ModeTiles", "Tipos de casilla"))
            + SSegmentedControl<EGridEditMode>::Slot(EGridEditMode::DeployZones)
            .Text(LOCTEXT("ModeZones", "Zonas de despliegue"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, 4.f)
        [
            SNew(STextBlock)
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .Text_Lambda([WeakAsset]()
            {
                if (GEditMode == EGridEditMode::TileTypes)
                {
                    return LOCTEXT("GridHintTiles", "Clic izquierdo: siguiente tipo · Clic derecho: tipo anterior");
                }

                const ULevelDataAsset* Asset = WeakAsset.Get();
                if (!Asset || Asset->DeployZones.Num() == 0)
                {
                    return LOCTEXT("GridHintNoZones", "Pulsa \"+ Zona\" para crear la primera zona de despliegue");
                }
                return LOCTEXT("GridHintZones", "Clic izquierdo: añadir a la zona activa · Clic derecho: quitar de su zona. Nombre y máximo se editan en Deploy Zones");
            })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, 0.f, 0.f, 6.f)
        [
            SNew(SBox)
            .Visibility_Lambda([]()
            {
                return GEditMode == EGridEditMode::DeployZones ? EVisibility::Visible : EVisibility::Collapsed;
            })
            [
                ZoneBar
            ]
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
