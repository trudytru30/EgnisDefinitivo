#include "Map/LevelMapManager.h"
#include "Map/MapGraphDataAsset.h"
#include "Map/MapNode.h"
#include "Map/MapProgressionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void ALevelMapManager::BeginPlay()
{
    Super::BeginPlay();
    Progression = GetGameInstance()->GetSubsystem<UMapProgressionSubsystem>();
    FString Error;
    if (!GraphDefinition)
        Error = TEXT("Assign a MapGraphDataAsset to GraphDefinition.");
    else if (!Progression)
        Error = TEXT("Map progression subsystem is unavailable.");
    else if (GraphDefinition->ValidateDefinition(Error) && BuildNodes(Error))
    {
        TArray<FMapNodeData> Definitions;
        GraphDefinition->GetNodeData(Definitions);
        MapID = GraphDefinition->MapID;
        RootNodeID = GraphDefinition->RootNodeID;
        bInitialized = Progression->InitializeMap(MapID, Definitions, RootNodeID, Error);
    }
    if (!bInitialized)
    {
        ClearNodes();
        UE_LOG(LogTemp, Error, TEXT("Level map initialization failed: %s"), *Error);
        OnMapInitializationFailed(Error);
        return;
    }
    Progression->OnProgressChanged.AddDynamic(this, &ALevelMapManager::RefreshNodes);
    RefreshNodes();
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
    }
}

bool ALevelMapManager::BuildNodes(FString& Error)
{
    for (const FMapNodeDefinition& Definition : GraphDefinition->Nodes)
    {
        const FTransform WorldTransform = Definition.LocalTransform * GetActorTransform();
        const TSubclassOf<AMapNode> Class = Definition.NodeClass ? Definition.NodeClass : GraphDefinition->DefaultNodeClass;
        AMapNode* Node = GetWorld()->SpawnActorDeferred<AMapNode>(Class, WorldTransform, this, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Node)
        {
            Error = FString::Printf(TEXT("Cannot spawn map node %s."), *Definition.Data.NodeID.ToString());
            return false;
        }
        Nodes.Add(Node);
        Node->Data = Definition.Data;
        Node->SetManager(this);
        Node->FinishSpawning(WorldTransform);
        if (!IsValid(Node))
        {
            Error = FString::Printf(TEXT("Node %s was destroyed during construction."), *Definition.Data.NodeID.ToString());
            return false;
        }
    }
    return true;
}

void ALevelMapManager::ClearNodes()
{
    for (AMapNode* Node : Nodes)
    {
        if (IsValid(Node))
        {
            Node->SetManager(nullptr);
            Node->Destroy();
        }
    }
    Nodes.Empty();
}

void ALevelMapManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bInitialized = false;
    if (Progression) Progression->OnProgressChanged.RemoveDynamic(this, &ALevelMapManager::RefreshNodes);
    ClearNodes();
    Super::EndPlay(EndPlayReason);
}

void ALevelMapManager::RefreshNodes()
{
    for (AMapNode* Node : Nodes)
    {
        if (IsValid(Node)) Node->UpdateState(Progression->GetNodeState(Node->Data.NodeID));
    }
}

bool ALevelMapManager::TryEnterNode(FName NodeID)
{
    if (!bInitialized || !Progression) return false;
    AMapNode* Selected = nullptr;
    for (AMapNode* Node : Nodes)
    {
        if (IsValid(Node) && Node->Data.NodeID == NodeID) { Selected = Node; break; }
    }
    if (!Selected) return false;
    const TSoftObjectPtr<UWorld> TargetLevel = Selected->Data.Level;
    if (bOpenLevelOnSelection && TargetLevel.IsNull()) return false;
    if (!Progression->TryEnterNode(NodeID)) return false;
    OnNodeSelected(Selected);
    if (bOpenLevelOnSelection) UGameplayStatics::OpenLevelBySoftObjectPtr(this, TargetLevel);
    return true;
}
