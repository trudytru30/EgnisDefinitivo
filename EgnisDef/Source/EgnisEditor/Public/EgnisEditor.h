#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(HeadCrackEditor, All, All)

class FEgnisEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    void AddMenu(FMenuBarBuilder& MenuBarBuilder);
    void FillMenu(FMenuBuilder& MenuBuilder);

private:
    void RegisterDetailCustomizations();
    void UnregisterDetailCustomizations();
};
