#include "Map/MapProgressionSubsystem.h"

bool UMapProgressionSubsystem::InitializeMap(FName MapID, const TArray<FMapNodeData>& Nodes, FName RootNodeID, FString& Error)
{
    Error.Empty();
    TMap<FName, FMapNodeData> Candidate;
    if (MapID.IsNone() || RootNodeID.IsNone() || Nodes.IsEmpty())
    {
        Error = TEXT("Map ID, root ID and nodes are required.");
        return false;
    }
    for (const FMapNodeData& Node : Nodes)
    {
        if (Node.NodeID.IsNone() || Candidate.Contains(Node.NodeID) || Node.DifficultyLevel < 1)
        {
            Error = TEXT("Node IDs must be nonempty and unique; difficulty must be at least 1.");
            return false;
        }
        FMapNodeData StableNode = Node;
        // PIE remaps soft world references during travel; those prefixes are not map changes.
        if (!Node.Level.IsNull())
        {
            const FSoftObjectPath& Path = Node.Level.ToSoftObjectPath();
            StableNode.Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(
                UWorld::RemovePIEPrefix(Path.GetLongPackageName()) + TEXT(".") +
                UWorld::RemovePIEPrefix(Path.GetAssetName())));
        }
        Candidate.Add(Node.NodeID, StableNode);
    }
    if (!Candidate.Contains(RootNodeID))
    {
        Error = TEXT("Root node does not exist.");
        return false;
    }
    uint8 Sections = 0;
    for (const auto& Pair : Candidate)
    {
        const uint8 Section = static_cast<uint8>(Pair.Value.Section);
        if (Section > 2)
        {
            Error = TEXT("A node must belong to one of the three predefined sections.");
            return false;
        }
        Sections |= 1 << Section;
    }
    if (Sections != 7 || Candidate.FindChecked(RootNodeID).Section != EMapSection::First)
    {
        Error = TEXT("Maps require three predefined sections and a root in the first section.");
        return false;
    }
    TMap<FName, int32> InDegree;
    for (const auto& Pair : Candidate) InDegree.Add(Pair.Key, 0);
    for (const auto& Pair : Candidate)
    {
        TSet<FName> Seen;
        for (FName Next : Pair.Value.NextNodes)
        {
            if (!Candidate.Contains(Next) || Next == Pair.Key || Seen.Contains(Next))
            {
                Error = FString::Printf(TEXT("Invalid or repeated connection from %s to %s."), *Pair.Key.ToString(), *Next.ToString());
                return false;
            }
            const int32 SectionStep = static_cast<int32>(Candidate.FindChecked(Next).Section) -
                static_cast<int32>(Pair.Value.Section);
            if (SectionStep < 0 || SectionStep > 1)
            {
                Error = TEXT("Connections cannot go back to an earlier section or skip a section.");
                return false;
            }
            Seen.Add(Next);
            ++InDegree.FindChecked(Next);
        }
    }
    TArray<FName> Queue;
    for (const auto& Pair : InDegree)
    {
        if (Pair.Value == 0) Queue.Add(Pair.Key);
    }
    if (Queue.Num() != 1 || Queue[0] != RootNodeID)
    {
        Error = TEXT("The map must have exactly one root, matching RootNodeID.");
        return false;
    }
    for (int32 Index = 0; Index < Queue.Num(); ++Index)
    {
        for (FName Next : Candidate.FindChecked(Queue[Index]).NextNodes)
        {
            if (--InDegree.FindChecked(Next) == 0) Queue.Add(Next);
        }
    }
    if (Queue.Num() != Candidate.Num())
    {
        Error = TEXT("The map contains a cycle or an unreachable component.");
        return false;
    }

    // Preserve progress on return only if the same map definition is still in use.
    bool bSameGraph = ActiveMapID == MapID && RootID == RootNodeID && Graph.Num() == Candidate.Num();
    if (bSameGraph)
    {
        for (const auto& Pair : Candidate)
        {
            const FMapNodeData* Old = Graph.Find(Pair.Key);
            const FMapNodeData& New = Pair.Value;
            if (!Old || Old->NextNodes != New.NextNodes || Old->NodeType != New.NodeType ||
                Old->DifficultyLevel != New.DifficultyLevel || Old->Section != New.Section || Old->Level != New.Level)
            {
                bSameGraph = false;
                break;
            }
        }
    }
    Graph = MoveTemp(Candidate);
    ActiveMapID = MapID;
    RootID = RootNodeID;
    if (!bSameGraph) ResetProgress();
    else OnProgressChanged.Broadcast();
    return true;
}

void UMapProgressionSubsystem::ResetProgress()
{
    CompletedNodes.Empty();
    CurrentNodeID = NAME_None;
    OnProgressChanged.Broadcast();
}

bool UMapProgressionSubsystem::IsNodeAccessible(FName NodeID) const
{
    if (!Graph.Contains(NodeID) || CompletedNodes.Contains(NodeID)) return false;
    if (CurrentNodeID.IsNone()) return NodeID == RootID;
    if (!CompletedNodes.Contains(CurrentNodeID)) return false;
    const FMapNodeData* Current = Graph.Find(CurrentNodeID);
    return Current && Current->NextNodes.Contains(NodeID);
}

EMapNodeState UMapProgressionSubsystem::GetNodeState(FName NodeID) const
{
    if (CompletedNodes.Contains(NodeID)) return EMapNodeState::Completed;
    if (Graph.Contains(NodeID) && NodeID == CurrentNodeID) return EMapNodeState::InProgress;
    return IsNodeAccessible(NodeID) ? EMapNodeState::Available : EMapNodeState::Locked;
}

bool UMapProgressionSubsystem::GetNodeData(FName NodeID, FMapNodeData& Data) const
{
    if (const FMapNodeData* Found = Graph.Find(NodeID))
    {
        Data = *Found;
        return true;
    }
    Data = FMapNodeData();
    return false;
}

bool UMapProgressionSubsystem::TryEnterNode(FName NodeID)
{
    if (!IsNodeAccessible(NodeID)) return false;
    CurrentNodeID = NodeID;
    OnProgressChanged.Broadcast();
    return true;
}

bool UMapProgressionSubsystem::CompleteCurrentNode()
{
    if (CurrentNodeID.IsNone() || !Graph.Contains(CurrentNodeID) || CompletedNodes.Contains(CurrentNodeID)) return false;
    CompletedNodes.Add(CurrentNodeID);
    OnProgressChanged.Broadcast();
    return true;
}


