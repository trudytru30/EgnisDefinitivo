#include "Map/LevelMapManager.h"
#include "Map/MapNode.h"
#include "Map/MapProgressionSubsystem.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void ALevelMapManager::BeginPlay()
{
    Super::BeginPlay();
    Progression = GetGameInstance()->GetSubsystem<UMapProgressionSubsystem>();
    TArray<FMapNodeData> Definitions;
    for (TActorIterator<AMapNode> It(GetWorld()); It; ++It)
    {
        Nodes.Add(*It);
        Definitions.Add(It->Data);
    }
    FString Error;
    bInitialized = Progression && Progression->InitializeMap(MapID, Definitions, RootNodeID, Error);
    if (!bInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("Level map initialization failed: %s"), *Error);
        OnMapInitializationFailed(Error);
        return;
    }
    for (AMapNode* Node : Nodes) Node->SetManager(this);
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

void ALevelMapManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Progression) Progression->OnProgressChanged.RemoveDynamic(this, &ALevelMapManager::RefreshNodes);
    for (AMapNode* Node : Nodes)
    {
        if (IsValid(Node)) Node->SetManager(nullptr);
    }
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
    // Do not consume the selection if its destination has not been configured.
    if (bOpenLevelOnSelection && TargetLevel.IsNull()) return false;
    if (!Progression->TryEnterNode(NodeID)) return false;
    OnNodeSelected(Selected);
    if (bOpenLevelOnSelection) UGameplayStatics::OpenLevelBySoftObjectPtr(this, TargetLevel);
    return true;
}
