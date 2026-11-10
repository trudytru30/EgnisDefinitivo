#include "MapBlueprintMigrationLibrary.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Engine/World.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Modules/ModuleManager.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Map/MapNode.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, EgnisDefEditor)

void UMapBlueprintMigrationLibrary::ClearLegacyLogic(UBlueprint* Blueprint, bool bRemoveVariables, bool bKeepConstruction)
{
    if (!Blueprint) return;
    Blueprint->Modify();
    const auto Interfaces = Blueprint->ImplementedInterfaces;
    for (const auto& Interface : Interfaces)
    {
        if (Interface.Interface)
            FBlueprintEditorUtils::RemoveInterface(Blueprint, Interface.Interface->GetClassPathName(), false);
    }
    TArray<UEdGraph*> Graphs;
    Graphs.Append(Blueprint->UbergraphPages);
    Graphs.Append(Blueprint->FunctionGraphs);
    Graphs.Append(Blueprint->MacroGraphs);
    for (UEdGraph* Graph : Graphs)
    {
        if (Graph && (!bKeepConstruction || Graph->GetFName() != TEXT("UserConstructionScript")))
            FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph);
    }
    if (bRemoveVariables)
    {
        TArray<FName> Names;
        for (const FBPVariableDescription& Variable : Blueprint->NewVariables) Names.Add(Variable.VarName);
        FBlueprintEditorUtils::BulkRemoveMemberVariables(Blueprint, Names);
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

void UMapBlueprintMigrationLibrary::ClearLevelLogic(UWorld* World)
{
    if (!World || !World->PersistentLevel) return;
    if (UBlueprint* BP = World->PersistentLevel->GetLevelScriptBlueprint(true))
    {
        ClearLegacyLogic(BP, true, false);
        FKismetEditorUtilities::CompileBlueprint(BP);
    }
}

bool UMapBlueprintMigrationLibrary::IsBlueprintValid(UBlueprint* Blueprint)
{
    return Blueprint && Blueprint->Status != BS_Error && Blueprint->GeneratedClass;
}

FString UMapBlueprintMigrationLibrary::ExportGraphSummary(UBlueprint* Blueprint)
{
    FString Out;
    if (!Blueprint) return Out;
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            Out += FString::Printf(TEXT("%s: %s\n"), *Graph->GetName(), *Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin->DefaultValue.IsEmpty())
                    Out += FString::Printf(TEXT("  %s=%s\n"), *Pin->PinName.ToString(), *Pin->DefaultValue);
            }
        }
    }
    return Out;
}



void UMapBlueprintMigrationLibrary::RepairComponentParents(UBlueprint* Blueprint)
{
    if (!Blueprint || !Blueprint->SimpleConstructionScript) return;
    Blueprint->Modify();
    USceneComponent* Root = GetDefault<AMapNode>()->GetRootComponent();
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetRootNodes())
    {
        Node->Modify();
        Node->SetParent(Root);
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}
