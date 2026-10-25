#include "Map/MapGraphDataAsset.h"
#include "Map/MapNode.h"
#include "Map/MapProgressionSubsystem.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void UMapGraphDataAsset::GetNodeData(TArray<FMapNodeData>& OutNodes) const
{
    OutNodes.Reset(Nodes.Num());
    for (const FMapNodeDefinition& Node : Nodes) OutNodes.Add(Node.Data);
}

bool UMapGraphDataAsset::ValidateDefinition(FString& Error) const
{
    for (const FMapNodeDefinition& Node : Nodes)
    {
        UClass* Class = Node.NodeClass ? Node.NodeClass.Get() : DefaultNodeClass.Get();
        if (!Class || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            Error = FString::Printf(TEXT("Node %s requires a concrete MapNode Blueprint/class (override or DefaultNodeClass)."),
                *Node.Data.NodeID.ToString());
            return false;
        }
        if (!Node.LocalTransform.IsValid() || Node.LocalTransform.GetScale3D().GetAbsMin() <= SMALL_NUMBER)
        {
            Error = FString::Printf(TEXT("Node %s has an invalid transform or zero scale."), *Node.Data.NodeID.ToString());
            return false;
        }
    }
    TArray<FMapNodeData> Definitions;
    GetNodeData(Definitions);
    return UMapProgressionSubsystem::ValidateMapDefinition(MapID, Definitions, RootNodeID, Error);
}

#if WITH_EDITOR
EDataValidationResult UMapGraphDataAsset::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    FString Error;
    if (!ValidateDefinition(Error))
    {
        Context.AddError(FText::FromString(Error));
        return EDataValidationResult::Invalid;
    }
    return ParentResult == EDataValidationResult::Invalid ? ParentResult : EDataValidationResult::Valid;
}
#endif
