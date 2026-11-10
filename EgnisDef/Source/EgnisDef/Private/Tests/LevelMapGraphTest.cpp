#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Map/MapProgressionSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelMapGraphTest, "Egnis.Map.GraphProgression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLevelMapGraphTest::RunTest(const FString& Parameters)
{
    UMapProgressionSubsystem* Map = NewObject<UMapProgressionSubsystem>(NewObject<UGameInstance>());
    TArray<FMapNodeData> Nodes;
    for (FName ID : { FName(TEXT("Start")), FName(TEXT("A")), FName(TEXT("B")), FName(TEXT("Boss")) })
    {
        FMapNodeData Node;
        Node.NodeID = ID;
        Nodes.Add(Node);
    }
    Nodes[1].Section = EMapSection::Second;
    Nodes[2].Section = EMapSection::Second;
    Nodes[3].Section = EMapSection::Third;
    Nodes[0].NextNodes = { TEXT("A"), TEXT("B") };
    Nodes[1].NextNodes = { TEXT("Boss") };
    Nodes[2].NextNodes = { TEXT("Boss") };
    Nodes[3].NodeType = EMapNodeType::Boss;
    Nodes[0].Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/MapTests/UEDPIE_0_Encounter.Encounter")));
    FString Error;
    TestTrue(TEXT("Accept branching and merging"), Map->InitializeMap(TEXT("Test"), Nodes, TEXT("Start"), Error));
    TestFalse(TEXT("Cannot skip root"), Map->TryEnterNode(TEXT("A")));
    TestTrue(TEXT("Enter root"), Map->TryEnterNode(TEXT("Start")));
    TestFalse(TEXT("Cannot advance before completion"), Map->TryEnterNode(TEXT("A")));
    TestTrue(TEXT("Complete root"), Map->CompleteCurrentNode());
    TestTrue(TEXT("Both branches available"), Map->IsNodeAccessible(TEXT("A")) && Map->IsNodeAccessible(TEXT("B")));
    TestTrue(TEXT("Select branch A"), Map->TryEnterNode(TEXT("A")));
    TestTrue(TEXT("Complete A"), Map->CompleteCurrentNode());
    TestFalse(TEXT("Cannot jump to sibling"), Map->TryEnterNode(TEXT("B")));
    Nodes[0].Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/MapTests/Encounter.Encounter")));
    TestTrue(TEXT("Restore same map"), Map->InitializeMap(TEXT("Test"), Nodes, TEXT("Start"), Error));
    TestEqual(TEXT("Preserve current node"), Map->GetCurrentNode(), FName(TEXT("A")));
    TestTrue(TEXT("Merge destination available"), Map->TryEnterNode(TEXT("Boss")));
    TestTrue(TEXT("Complete final node"), Map->CompleteCurrentNode());
    TestFalse(TEXT("Cannot replay completed root"), Map->TryEnterNode(TEXT("Start")));
    Nodes[3].NextNodes = { TEXT("A") };
    TestFalse(TEXT("Reject cycle"), Map->InitializeMap(TEXT("Invalid"), Nodes, TEXT("Start"), Error));
    TestEqual(TEXT("Invalid graph preserves progress"), Map->GetCurrentNode(), FName(TEXT("Boss")));
    Nodes[3].NextNodes = { TEXT("Missing") };
    TestFalse(TEXT("Reject missing destination"), Map->InitializeMap(TEXT("Invalid"), Nodes, TEXT("Start"), Error));
    Nodes[3].NextNodes.Empty();
    Nodes[2].NodeID = TEXT("A");
    TestFalse(TEXT("Reject duplicate IDs"), Map->InitializeMap(TEXT("Invalid"), Nodes, TEXT("Start"), Error));
    Nodes[2].NodeID = TEXT("B");
    Nodes[0].NextNodes.Add(TEXT("Boss"));
    TestFalse(TEXT("Reject skipping section two"), Map->InitializeMap(TEXT("Invalid"), Nodes, TEXT("Start"), Error));
    Nodes[0].NextNodes.Remove(TEXT("Boss"));
    Nodes[3].Section = EMapSection::Second;
    TestFalse(TEXT("Require all three sections"), Map->InitializeMap(TEXT("Invalid"), Nodes, TEXT("Start"), Error));
    Map->ResetProgress();
    TestTrue(TEXT("Reset makes root available"), Map->IsNodeAccessible(TEXT("Start")));
    return true;
}
#endif



