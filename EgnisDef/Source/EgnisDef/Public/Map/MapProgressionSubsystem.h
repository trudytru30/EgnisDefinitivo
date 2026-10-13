#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Map/MapTypes.h"
#include "MapProgressionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMapProgressChanged);

UCLASS()
class EGNISDEF_API UMapProgressionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Validates a single-root directed acyclic graph before replacing any state.
    UFUNCTION(BlueprintCallable, Category="Map")
    bool InitializeMap(FName MapID, const TArray<FMapNodeData>& Nodes, FName RootNodeID, FString& Error);

    UFUNCTION(BlueprintCallable, Category="Map")
    void ResetProgress();

    UFUNCTION(BlueprintPure, Category="Map")
    bool IsNodeAccessible(FName NodeID) const;

    UFUNCTION(BlueprintPure, Category="Map")
    EMapNodeState GetNodeState(FName NodeID) const;

    UFUNCTION(BlueprintPure, Category="Map")
    FName GetCurrentNode() const { return CurrentNodeID; }

    UFUNCTION(BlueprintPure, Category="Map")
    bool GetNodeData(FName NodeID, FMapNodeData& Data) const;

    UFUNCTION(BlueprintCallable, Category="Map")
    bool TryEnterNode(FName NodeID);

    // Call after winning a battle / resolving an event, before returning to the map.
    UFUNCTION(BlueprintCallable, Category="Map")
    bool CompleteCurrentNode();

    UPROPERTY(BlueprintAssignable, Category="Map")
    FMapProgressChanged OnProgressChanged;

private:
    UPROPERTY()
    TMap<FName, FMapNodeData> Graph;

    UPROPERTY()
    TSet<FName> CompletedNodes;

    UPROPERTY()
    FName ActiveMapID;

    UPROPERTY()
    FName RootID;

    UPROPERTY()
    FName CurrentNodeID;
};
