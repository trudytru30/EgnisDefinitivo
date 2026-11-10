#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "InputCoreTypes.h"
#include "Map/MapNode.h"
#include "Map/MapGraphDataAsset.h"
#include "Map/LevelMapManager.h"
#include "Map/MapEncounterExit.h"
#include "Map/MapProgressionSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapBlueprintIntegrationTest, "Egnis.Map.BlueprintIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static AMapNode* FindNode(UWorld* World, FName ID)
{
    for (TActorIterator<AMapNode> It(World); It; ++It)
        if (It->Data.NodeID == ID) return *It;
    return nullptr;
}

static bool ClickVisualNode(AMapNode* Node)
{
    if (!Node) return false;
    TArray<UPrimitiveComponent*> Components;
    Node->GetComponents(Components);
    for (UPrimitiveComponent* Component : Components)
    {
        if (Component->GetFName() == TEXT("Sphere"))
        {
            Component->OnClicked.Broadcast(Component, EKeys::LeftMouseButton);
            return true;
        }
    }
    return false;
}

class FCheckMapStage : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    int32 Stage;
    double Started = FPlatformTime::Seconds();
public:
    FCheckMapStage(FAutomationTestBase* InTest, int32 InStage) : Test(InTest), Stage(InStage) {}
    virtual bool Update() override
    {
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        const bool bExpected = World && (Stage == 1 ? World->GetName().Contains(TEXT("L_TestPartida")) :
            World->GetName().Contains(TEXT("MapaIntercativo")));
        if (!bExpected || !World->HasBegunPlay())
        {
            if (FPlatformTime::Seconds() - Started < 45.0) return false;
            Test->AddError(FString::Printf(TEXT("Timeout in map integration stage %d"), Stage));
            return true;
        }
        auto* Progression = World->GetGameInstance()->GetSubsystem<UMapProgressionSubsystem>();
        if (Stage == 1)
        {
            Test->TestEqual(TEXT("Current node survives travel"), Progression->GetCurrentNode(), FName(TEXT("Diff1")));
            for (TActorIterator<AMapEncounterExit> It(World); It; ++It)
            {
                Test->TestTrue(TEXT("Complete prototype and return"), It->CompleteAndReturn());
                return true;
            }
            Test->AddError(TEXT("Prototype exit actor missing"));
            return true;
        }
        ALevelMapManager* Manager = nullptr;
        for (TActorIterator<ALevelMapManager> It(World); It; ++It) { Manager = *It; break; }
        if (!Test->TestNotNull(TEXT("Migrated Blueprint manager"), Manager)) return true;
        AMapNode* Root = FindNode(World, TEXT("Diff1"));
        if (Stage == 0)
        {
            Test->TestNotNull(TEXT("Manager reads a graph Data Asset"), Manager->GraphDefinition.Get());
            if (Manager->GraphDefinition)
                Test->TestEqual(TEXT("Asset supplies 30 definitions"), Manager->GraphDefinition->Nodes.Num(), 30);
            if (Root) Test->TestEqual(TEXT("Root was spawned by manager"), Root->GetOwner(), static_cast<AActor*>(Manager));
            int32 Count = 0;
            for (TActorIterator<AMapNode> It(World); It; ++It) ++Count;
            Test->TestEqual(TEXT("All original nodes migrated"), Count, 30);
            Test->TestTrue(TEXT("Root is available"), Progression->IsNodeAccessible(TEXT("Diff1")));
            Test->TestFalse(TEXT("Cannot skip root"), Manager->TryEnterNode(TEXT("Diff2")));
            Test->TestTrue(TEXT("Original visual sphere handles click"), ClickVisualNode(Root));
            Test->TestEqual(TEXT("Click selects root"), Progression->GetCurrentNode(), FName(TEXT("Diff1")));
        }
        else
        {
            Test->TestEqual(TEXT("Completed root after return"), Progression->GetNodeState(TEXT("Diff1")), EMapNodeState::Completed);
            Test->TestTrue(TEXT("Direct child available"), Progression->IsNodeAccessible(TEXT("Diff2")));
            Test->TestFalse(TEXT("Later node still locked"), Progression->IsNodeAccessible(TEXT("B1")));
            Manager->bOpenLevelOnSelection = false;
            Test->TestTrue(TEXT("Click next original Blueprint"), ClickVisualNode(FindNode(World, TEXT("Diff2"))));
            Test->TestEqual(TEXT("Selected direct child"), Progression->GetCurrentNode(), FName(TEXT("Diff2")));
            Test->TestFalse(TEXT("Sibling cannot be selected"), Manager->TryEnterNode(TEXT("Diff3")));
        }
        return true;
    }
};

class FVerifyMapSourceCommand : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
public:
    explicit FVerifyMapSourceCommand(FAutomationTestBase* InTest) : Test(InTest) {}
    bool Update() override
    {
        UWorld* World = GEditor->GetEditorWorldContext().World();
        int32 PlacedNodes = 0;
        for (TActorIterator<AMapNode> It(World); It; ++It) ++PlacedNodes;
        Test->TestEqual(TEXT("Map does not contain hand-placed casillas"), PlacedNodes, 0);
        return true;
    }
};
bool FMapBlueprintIntegrationTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/GameDesign/Luis/Level_Proto/MapaJuego/MapaIntercativo")));
    ADD_LATENT_AUTOMATION_COMMAND(FVerifyMapSourceCommand(this));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FCheckMapStage(this, 0));
    ADD_LATENT_AUTOMATION_COMMAND(FCheckMapStage(this, 1));
    ADD_LATENT_AUTOMATION_COMMAND(FCheckMapStage(this, 2));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
