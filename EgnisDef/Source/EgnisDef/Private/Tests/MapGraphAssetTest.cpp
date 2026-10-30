#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Map/MapGraphDataAsset.h"
#include "Map/MapNode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapGraphAssetTest, "Egnis.Map.DataAssetValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapGraphAssetTest::RunTest(const FString& Parameters)
{
    UMapGraphDataAsset* Asset = NewObject<UMapGraphDataAsset>();
    Asset->MapID = TEXT("AssetTest");
    Asset->RootNodeID = TEXT("Start");
    Asset->DefaultNodeClass = AMapNode::StaticClass();
    FMapNodeDefinition Start, Middle, Boss;
    Start.Data.NodeID = TEXT("Start");
    Start.Data.NextNodes = { TEXT("Middle") };
    Middle.Data.NodeID = TEXT("Middle");
    Middle.Data.Section = EMapSection::Second;
    Middle.Data.NextNodes = { TEXT("Boss") };
    Boss.Data.NodeID = TEXT("Boss");
    Boss.Data.Section = EMapSection::Third;
    Boss.Data.NodeType = EMapNodeType::Boss;
    Asset->Nodes = { Start, Middle, Boss };
    FString Error;
    TestTrue(TEXT("Valid three-section asset"), Asset->ValidateDefinition(Error));
    Asset->Nodes[0].Data.NextNodes = { TEXT("Missing") };
    TestFalse(TEXT("Reject dangling IDs in asset"), Asset->ValidateDefinition(Error));
    Asset->Nodes[0].Data.NextNodes = { TEXT("Boss") };
    TestFalse(TEXT("Reject skipped section"), Asset->ValidateDefinition(Error));
    Asset->Nodes[0] = Start;
    Asset->Nodes[1].LocalTransform.SetScale3D(FVector::ZeroVector);
    TestFalse(TEXT("Reject zero-scale node"), Asset->ValidateDefinition(Error));
    Asset->Nodes[1] = Middle;
    Asset->DefaultNodeClass = nullptr;
    TestFalse(TEXT("Require node presentation class"), Asset->ValidateDefinition(Error));
    for (FMapNodeDefinition& Node : Asset->Nodes) Node.NodeClass = AMapNode::StaticClass();
    TestTrue(TEXT("Per-node classes can replace default"), Asset->ValidateDefinition(Error));
    TArray<FMapNodeData> Data;
    Asset->GetNodeData(Data);
    TestEqual(TEXT("Asset provides all nodes"), Data.Num(), 3);
    TestEqual(TEXT("Asset provides topology"), Data[0].NextNodes[0], FName(TEXT("Middle")));
    return true;
}
#endif
