#include "LevelDataAssetCustomization.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "EgnisBlockoutImporter.h"
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
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LevelDataAssetCustomization"

namespace
{
    constexpr float CellWidth = 44.f;
    constexpr float CellHeight = 44.f;

    enum class EGridEditMode : uint8
    {
        TileTypes,
        Placement,
        Objectives
    };

    enum class EObjectiveTool : uint8
    {
        EscortStart,
        EscortEnd,
        Protect,
        Destroy
    };

    enum class EObjectiveAt : uint8
    {
        None,
        EscortStart,
        EscortEnd,
        Protect,
        Destroy
    };

    // Se guardan fuera de la customización porque ésta se vuelve a crear cada vez que se refresca el panel
    EGridEditMode GEditMode = EGridEditMode::TileTypes;
    EObjectiveTool GObjectiveTool = EObjectiveTool::EscortStart;

    const FLinearColor DeployAllowedColor(0.20f, 1.00f, 0.30f);
    const FLinearColor DeployBlockedColor(1.00f, 0.15f, 0.15f);

    ETileType GetTile(const ULevelDataAsset* Asset, int32 X, int32 Y)
    {
        return Asset ? Asset->GetTileType(FTileCoord(X, Y)) : ETileType::Void;
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
        case ETileType::Limited:           return FLinearColor(0.85f, 0.75f, 0.35f);
        default:                           return FLinearColor::White;
        }
    }

    const FEnemySpawnInfo* FindEnemy(const ULevelDataAsset* Asset, int32 X, int32 Y)
    {
        return Asset ? Asset->EnemiesToSpawn.FindByPredicate([X, Y](const FEnemySpawnInfo& Enemy)
        {
            return Enemy.SpawnCoord.X == X && Enemy.SpawnCoord.Y == Y;
        }) : nullptr;
    }

    FText GetEnemyName(const FEnemySpawnInfo& Enemy)
    {
        if (!Enemy.EnemyClass)
        {
            return LOCTEXT("EnemyWithoutClass", "Enemigo ?");
        }

        FString Name = Enemy.EnemyClass->GetName();
        Name.RemoveFromEnd(TEXT("_C"));
        Name.RemoveFromStart(TEXT("BP_Enemy_"));
        return FText::FromString(Name);
    }

    EObjectiveAt GetObjective(const ULevelDataAsset* Asset, int32 X, int32 Y)
    {
        if (!Asset)
        {
            return EObjectiveAt::None;
        }

        const FSecondaryObjectives& Objectives = Asset->SecondaryObjectives;
        const FTileCoord Tile(X, Y);
        if (Objectives.bEscort && Objectives.EscortStart == Tile)
        {
            return EObjectiveAt::EscortStart;
        }
        if (Objectives.bEscort && Objectives.EscortEnd == Tile)
        {
            return EObjectiveAt::EscortEnd;
        }
        if (Objectives.TilesToProtect.Contains(Tile))
        {
            return EObjectiveAt::Protect;
        }
        if (Objectives.TilesToDestroy.Contains(Tile))
        {
            return EObjectiveAt::Destroy;
        }
        return EObjectiveAt::None;
    }

    FText GetObjectiveLabel(EObjectiveAt Objective)
    {
        switch (Objective)
        {
        case EObjectiveAt::EscortStart: return LOCTEXT("ObjA", "A");
        case EObjectiveAt::EscortEnd:   return LOCTEXT("ObjB", "B");
        case EObjectiveAt::Protect:     return LOCTEXT("ObjProtect", "Proteger");
        case EObjectiveAt::Destroy:     return LOCTEXT("ObjDestroy", "Destruir");
        default:                        return FText::GetEmpty();
        }
    }

    FLinearColor GetObjectiveColor(EObjectiveAt Objective)
    {
        switch (Objective)
        {
        case EObjectiveAt::EscortStart: return FLinearColor(0.10f, 0.85f, 1.00f);
        case EObjectiveAt::EscortEnd:   return FLinearColor(0.30f, 0.45f, 1.00f);
        case EObjectiveAt::Protect:     return FLinearColor(1.00f, 0.85f, 0.10f);
        case EObjectiveAt::Destroy:     return FLinearColor(1.00f, 0.40f, 0.10f);
        default:                        return FLinearColor::Transparent;
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

    // Una casilla solo tiene un objetivo: se quita el que hubiera (si era A o B, la escolta deja de estar completa)
    void RemoveObjectiveAt(FSecondaryObjectives& Objectives, const FTileCoord& Tile)
    {
        if (Objectives.bEscort && (Objectives.EscortStart == Tile || Objectives.EscortEnd == Tile))
        {
            Objectives.bEscort = false;
        }
        Objectives.TilesToProtect.Remove(Tile);
        Objectives.TilesToDestroy.Remove(Tile);
    }

    void SetObjective(ULevelDataAsset* Asset, int32 X, int32 Y, EObjectiveTool Tool)
    {
        if (!Asset)
        {
            return;
        }

        const FScopedTransaction Transaction(LOCTEXT("SetObjective", "Poner objetivo secundario"));
        Asset->Modify();

        FSecondaryObjectives& Objectives = Asset->SecondaryObjectives;
        const FTileCoord Tile(X, Y);
        RemoveObjectiveAt(Objectives, Tile);

        switch (Tool)
        {
        case EObjectiveTool::EscortStart:
            if (!Objectives.bEscort)
            {
                Objectives.EscortEnd = Tile;
            }
            Objectives.EscortStart = Tile;
            Objectives.bEscort = true;
            break;
        case EObjectiveTool::EscortEnd:
            if (!Objectives.bEscort)
            {
                Objectives.EscortStart = Tile;
            }
            Objectives.EscortEnd = Tile;
            Objectives.bEscort = true;
            break;
        case EObjectiveTool::Protect:
            Objectives.TilesToProtect.AddUnique(Tile);
            break;
        case EObjectiveTool::Destroy:
            Objectives.TilesToDestroy.AddUnique(Tile);
            break;
        }
    }

    void ClearObjective(ULevelDataAsset* Asset, int32 X, int32 Y)
    {
        if (!Asset || GetObjective(Asset, X, Y) == EObjectiveAt::None)
        {
            return;
        }

        const FScopedTransaction Transaction(LOCTEXT("ClearObjective", "Quitar objetivo secundario"));
        Asset->Modify();
        RemoveObjectiveAt(Asset->SecondaryObjectives, FTileCoord(X, Y));
    }

    void RefreshDetails(const TWeakPtr<IDetailLayoutBuilder>& WeakBuilder)
    {
        if (TSharedPtr<IDetailLayoutBuilder> Builder = WeakBuilder.Pin())
        {
            Builder->ForceRefreshDetails();
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

    const TWeakPtr<IDetailLayoutBuilder> WeakBuilder = CachedDetailBuilder;
    const FSimpleDelegate RefreshGrid = FSimpleDelegate::CreateLambda([WeakBuilder]() { RefreshDetails(WeakBuilder); });
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
                    // Marco: en Colocación verde/rojo, en Objetivos el color del objetivo y en Tipos de casilla rojo si hay un enemigo
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
                    .Padding(4.f)
                    .BorderBackgroundColor_Lambda([WeakAsset, X, Y]()
                    {
                        const ULevelDataAsset* Asset = WeakAsset.Get();
                        if (!Asset)
                        {
                            return FSlateColor(FLinearColor::Transparent);
                        }

                        switch (GEditMode)
                        {
                        case EGridEditMode::Placement:
                            if (Asset->CanDeployOnTile(FTileCoord(X, Y)))
                            {
                                return FSlateColor(DeployAllowedColor);
                            }
                            return FSlateColor(ULevelDataAsset::IsDeployableTileType(GetTile(Asset, X, Y)) || FindEnemy(Asset, X, Y)
                                ? DeployBlockedColor : FLinearColor::Transparent);
                        case EGridEditMode::Objectives:
                            return FSlateColor(GetObjectiveColor(GetObjective(Asset, X, Y)));
                        default:
                            return FSlateColor(FindEnemy(Asset, X, Y) ? DeployBlockedColor : FLinearColor::Transparent);
                        }
                    })
                    .ToolTipText_Lambda([WeakAsset, X, Y]()
                    {
                        const ULevelDataAsset* Asset = WeakAsset.Get();
                        const FEnemySpawnInfo* Enemy = FindEnemy(Asset, X, Y);
                        const EObjectiveAt Objective = GetObjective(Asset, X, Y);
                        const bool bCanDeploy = Asset && Asset->CanDeployOnTile(FTileCoord(X, Y));
                        return FText::Format(LOCTEXT("TileTooltip", "({0}, {1}) {2}{3}{4}\n{5}"), X, Y, GetTileName(GetTile(Asset, X, Y)),
                            Enemy ? FText::Format(LOCTEXT("TooltipEnemy", "\nEnemigo: {0}"), GetEnemyName(*Enemy)) : FText::GetEmpty(),
                            Objective != EObjectiveAt::None ? FText::Format(LOCTEXT("TooltipObjective", "\nObjetivo: {0}"), GetObjectiveLabel(Objective)) : FText::GetEmpty(),
                            bCanDeploy ? LOCTEXT("TooltipCanDeploy", "Se pueden colocar personajes") : LOCTEXT("TooltipCannotDeploy", "No se pueden colocar personajes"));
                    })
                    .OnMouseButtonDown_Lambda([WeakAsset, X, Y](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
                    {
                        const bool bLeft = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
                        const bool bRight = MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
                        if ((!bLeft && !bRight) || GEditMode == EGridEditMode::Placement)
                        {
                            return FReply::Unhandled();
                        }

                        if (GEditMode == EGridEditMode::TileTypes)
                        {
                            CycleTile(WeakAsset.Get(), X, Y, bLeft ? 1 : -1);
                        }
                        else if (bLeft)
                        {
                            SetObjective(WeakAsset.Get(), X, Y, GObjectiveTool);
                        }
                        else
                        {
                            ClearObjective(WeakAsset.Get(), X, Y);
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
                                const FEnemySpawnInfo* Enemy = FindEnemy(Asset, X, Y);
                                if (GEditMode == EGridEditMode::Objectives)
                                {
                                    const EObjectiveAt Objective = GetObjective(Asset, X, Y);
                                    if (Objective != EObjectiveAt::None)
                                    {
                                        return GetObjectiveLabel(Objective);
                                    }
                                }
                                if (Enemy)
                                {
                                    return GetEnemyName(*Enemy);
                                }
                                return GEditMode == EGridEditMode::TileTypes ? GetTileName(GetTile(Asset, X, Y)) : FText::GetEmpty();
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

    const auto ImportButton = [](const FText& Text, const FText& ToolTip, TFunction<void()> Action, TFunction<bool()> IsEnabled)
    {
        return SNew(SButton)
            .Text(Text)
            .ToolTipText(ToolTip)
            .IsEnabled_Lambda([IsEnabled]() { return !IsEnabled || IsEnabled(); })
            .OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); });
    };

    TSharedRef<SHorizontalBox> ImportBar = SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.f, 0.f, 4.f, 0.f)
        [
            ImportButton(LOCTEXT("PasteJson", "Pegar JSON"),
                LOCTEXT("PasteJsonTip", "Importa este nivel desde el JSON copiado en Egnis Blockout (Export -> Ver JSON de todos -> Copy text)"),
                [WeakAsset, WeakBuilder]()
                {
                    FString Json;
                    if (ULevelDataAsset* Asset = WeakAsset.Get(); Asset && EgnisBlockout::ReadClipboard(Json) && EgnisBlockout::ImportIntoAsset(Asset, Json, FString()))
                    {
                        RefreshDetails(WeakBuilder);
                    }
                }, nullptr)
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.f, 0.f, 4.f, 0.f)
        [
            ImportButton(LOCTEXT("ImportFile", "Importar archivo..."),
                LOCTEXT("ImportFileTip", "Importa este nivel desde un .json descargado de Egnis Blockout y recuerda el archivo para reimportar"),
                [WeakAsset, WeakBuilder]()
                {
                    FString StoredPath;
                    FString Json;
                    if (ULevelDataAsset* Asset = WeakAsset.Get(); Asset && EgnisBlockout::PickJsonFile(StoredPath, Json) && EgnisBlockout::ImportIntoAsset(Asset, Json, StoredPath))
                    {
                        RefreshDetails(WeakBuilder);
                    }
                }, nullptr)
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.f, 0.f, 8.f, 0.f)
        [
            ImportButton(LOCTEXT("Reimport", "Reimportar"),
                LOCTEXT("ReimportTip", "Vuelve a leer el último archivo importado"),
                [WeakAsset, WeakBuilder]()
                {
                    if (EgnisBlockout::Reimport(WeakAsset.Get()))
                    {
                        RefreshDetails(WeakBuilder);
                    }
                },
                [WeakAsset]() { const ULevelDataAsset* Asset = WeakAsset.Get(); return Asset && !Asset->BlockoutSourceFile.IsEmpty(); })
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .Text_Lambda([WeakAsset]()
            {
                const ULevelDataAsset* Asset = WeakAsset.Get();
                if (!Asset || Asset->BlockoutLevelId.IsEmpty())
                {
                    return LOCTEXT("NotImported", "Sin importar de Egnis Blockout");
                }
                return FText::Format(LOCTEXT("ImportedFrom", "Egnis Blockout: {0} · {1}"), FText::FromString(Asset->BlockoutLevelName),
                    Asset->BlockoutSourceFile.IsEmpty() ? LOCTEXT("FromClipboard", "pegado") : FText::FromString(Asset->BlockoutSourceFile));
            })
        ];

    GridCategory.AddCustomRow(FText::FromString("GridMap"))
    .WholeRowContent()
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, 4.f)
        [
            ImportBar
        ]
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
            + SSegmentedControl<EGridEditMode>::Slot(EGridEditMode::Placement)
            .Text(LOCTEXT("ModePlacement", "Colocación"))
            + SSegmentedControl<EGridEditMode>::Slot(EGridEditMode::Objectives)
            .Text(LOCTEXT("ModeObjectives", "Objetivos"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Left)
        .Padding(0.f, 0.f, 0.f, 4.f)
        [
            SNew(SBox)
            .Visibility_Lambda([]() { return GEditMode == EGridEditMode::Objectives ? EVisibility::Visible : EVisibility::Collapsed; })
            [
                SNew(SSegmentedControl<EObjectiveTool>)
                .Value_Lambda([]() { return GObjectiveTool; })
                .OnValueChanged_Lambda([](EObjectiveTool NewTool) { GObjectiveTool = NewTool; })
                + SSegmentedControl<EObjectiveTool>::Slot(EObjectiveTool::EscortStart)
                .Text(LOCTEXT("ToolEscortStart", "Escolta A"))
                + SSegmentedControl<EObjectiveTool>::Slot(EObjectiveTool::EscortEnd)
                .Text(LOCTEXT("ToolEscortEnd", "Escolta B"))
                + SSegmentedControl<EObjectiveTool>::Slot(EObjectiveTool::Protect)
                .Text(LOCTEXT("ToolProtect", "Proteger"))
                + SSegmentedControl<EObjectiveTool>::Slot(EObjectiveTool::Destroy)
                .Text(LOCTEXT("ToolDestroy", "Destruir"))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, 0.f, 0.f, 6.f)
        [
            SNew(STextBlock)
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .AutoWrapText(true)
            .Text_Lambda([WeakAsset]()
            {
                switch (GEditMode)
                {
                case EGridEditMode::Placement:
                {
                    const ULevelDataAsset* Asset = WeakAsset.Get();
                    return FText::Format(LOCTEXT("GridHintPlacement", "Verde: se pueden colocar personajes. Rojo: a {0} casillas o menos de un enemigo (Deploy Min Distance To Enemies). Los enemigos se editan en Enemies To Spawn."),
                        Asset ? Asset->DeployMinDistanceToEnemies : 0);
                }
                case EGridEditMode::Objectives:
                    return LOCTEXT("GridHintObjectives", "Clic izquierdo: poner el objetivo elegido. Clic derecho: quitarlo. La escolta necesita A y B.");
                default:
                    return LOCTEXT("GridHintTiles", "Clic izquierdo: siguiente tipo. Clic derecho: tipo anterior. Borde rojo: hay un enemigo.");
                }
            })
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
