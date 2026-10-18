#include "CoreAndSystems/LevelDataAssetCustomization.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FLevelDataAssetCustomization::MakeInstance()
{
    return MakeShared<FLevelDataAssetCustomization>();
}

void FLevelDataAssetCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // Obtenemos una referencia al Data Asset que se está editando
    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);
    if (Objects.Num() == 0) return;

    ULevelDataAsset* LevelAsset = Cast<ULevelDataAsset>(Objects[0].Get());
    if (!LevelAsset) return;

    // Creamos una categoría limpia en el panel de detalles
    IDetailCategoryBuilder& GridCategory = DetailBuilder.EditCategory("VisualGrid", FText::FromString("Editor de Cuadrícula de Nivel"));

    // Contenedor visual en forma de rejilla (Grid)
    TSharedPtr<SGridPanel> GridPanel = SNew(SGridPanel);

    int32 Width = LevelAsset->Width;
    int32 Height = LevelAsset->Height;

    // Sincronizamos el tamaño de la matriz de datos por seguridad
    if (LevelAsset->GridRows.Num() != Height)
    {
        LevelAsset->GridRows.SetNum(Height);
        for (auto& Row : LevelAsset->GridRows)
        {
            Row.Columns.SetNum(Width);
        }
    }

    // Rellenamos la cuadrícula visual con botones organizados por coordenadas (X, Y)
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            GridPanel->AddSlot(X, Y)
            .Padding(2.0f)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .Text(FText::FromString(FString::Printf(TEXT("%d,%d"), X, Y)))
                .OnClicked_Lambda([LevelAsset, X, Y]() -> FReply {
                    // Aquí puedes programar qué pasa al hacer clic en una casilla 
                    // (por ejemplo, cambiar su tipo cíclicamente o abrir un menú desplegable)
                    return FReply::Handled();
                })
            ];
        }
    }

    // Inyectamos el panel visual en los detalles del editor
    GridCategory.AddCustomRow(FText::FromString("GridMap"))
    .NameContent()
    [
        SNew(STextBlock).Text(FText::FromString("Tablero Visual"))
    ]
    .ValueContent()
    .MinDesiredWidth(300.f)
    [
        GridPanel.ToSharedRef()
    ];
}