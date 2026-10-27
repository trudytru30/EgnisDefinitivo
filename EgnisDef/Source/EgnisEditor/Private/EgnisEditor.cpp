#include "EgnisEditor.h"
#include "LevelEditor.h"
#include "PropertyEditorModule.h"
#include "LevelDataAssetCustomization.h"
#include "CoreAndSystems/LevelDataAsset.h"

#define LOCTEXT_NAMESPACE "FEgnisEditorModule"
class UEditorUtilitySubsystem;
DEFINE_LOG_CATEGORY(HeadCrackEditor)

void FEgnisEditorModule::StartupModule()
{
    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

    MenuExtender->AddMenuBarExtension("Help", EExtensionHook::After, nullptr, FMenuBarExtensionDelegate::CreateRaw(this, &FEgnisEditorModule::AddMenu));
    LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

    RegisterDetailCustomizations();
}

void FEgnisEditorModule::RegisterDetailCustomizations()
{
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropertyModule.RegisterCustomClassLayout(
        ULevelDataAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FLevelDataAssetCustomization::MakeInstance));

    PropertyModule.NotifyCustomizationModuleChanged();
}

void FEgnisEditorModule::UnregisterDetailCustomizations()
{
    if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor") || !UObjectInitialized())
    {
        return;
    }

    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.UnregisterCustomClassLayout(ULevelDataAsset::StaticClass()->GetFName());
}

void FEgnisEditorModule::AddMenu(FMenuBarBuilder& MenuBarBuilder)
{
    MenuBarBuilder.AddPullDownMenu(FText::FromString("Sheets Importer"), FText::FromString("Google Sheets Tools"), FNewMenuDelegate::CreateRaw(this, &FEgnisEditorModule::FillMenu));
}

void FEgnisEditorModule::FillMenu(FMenuBuilder& MenuBuilder)
{
    MenuBuilder.AddMenuEntry(
        FText::FromString("Import Game Texts"),
        FText::FromString("Imports Text from Google Sheets"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([]()
        {
            
            FString BlueprintPath = TEXT("/Game/Tools/Menu_SyncSheets.Menu_SyncSheets_C");

            UClass* BlueprintClass = StaticLoadClass(UObject::StaticClass(), nullptr, *BlueprintPath);
            
            if (BlueprintClass)
            {
                UObject* BlueprintInstance = NewObject<UObject>(GetTransientPackage(), BlueprintClass);
                
                if (BlueprintInstance)
                {
                    UFunction* FunctionToRun = BlueprintInstance->FindFunction(FName("SyncTextSheets"));
                    
                    if (FunctionToRun)
                    {
                        BlueprintInstance->ProcessEvent(FunctionToRun, nullptr);
                        UE_LOG(HeadCrackEditor, Log, TEXT("Blueprint ejecutado correctamente"));
                    }
                    else
                    {
                        UE_LOG(HeadCrackEditor, Error, TEXT("No se encontró la función 'Run' en el Blueprint."));
                    }
                }
            }
            else
            {
                UE_LOG(HeadCrackEditor, Error, TEXT("No se pudo cargar la clase del Blueprint. Revisa la ruta."));
            }
        }))
    );
}

void FEgnisEditorModule::ShutdownModule()
{
    UnregisterDetailCustomizations();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEgnisEditorModule, EgnisEditor)