#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelMapManager.generated.h"

class AMapNode;
class UMapGraphDataAsset;
class UMapProgressionSubsystem;

UCLASS(Blueprintable)
class EGNISDEF_API ALevelMapManager : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    TObjectPtr<UMapGraphDataAsset> GraphDefinition;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Map")
    FName MapID = TEXT("Zone1");

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Map")
    FName RootNodeID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    bool bOpenLevelOnSelection = true;

    UFUNCTION(BlueprintCallable, Category="Map")
    bool TryEnterNode(FName NodeID);

    UFUNCTION(BlueprintImplementableEvent, Category="Map")
    void OnNodeSelected(AMapNode* Node);

    UFUNCTION(BlueprintImplementableEvent, Category="Map")
    void OnMapInitializationFailed(const FString& Error);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY()
    TObjectPtr<UMapProgressionSubsystem> Progression;

    UPROPERTY()
    TArray<TObjectPtr<AMapNode>> Nodes;

    bool bInitialized = false;
    bool BuildNodes(FString& Error);
    void ClearNodes();

    UFUNCTION()
    void RefreshNodes();
};
