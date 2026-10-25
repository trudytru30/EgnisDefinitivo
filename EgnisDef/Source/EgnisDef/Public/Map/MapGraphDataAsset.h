#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Map/MapTypes.h"
#include "MapGraphDataAsset.generated.h"

class AMapNode;

USTRUCT(BlueprintType)
struct EGNISDEF_API FMapNodeDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map", meta=(ShowOnlyInnerProperties))
    FMapNodeData Data;

    // Relative to the map manager, including rotation and scale.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    FTransform LocalTransform = FTransform::Identity;

    // Optional override of DefaultNodeClass.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    TSubclassOf<AMapNode> NodeClass;
};

UCLASS(BlueprintType)
class EGNISDEF_API UMapGraphDataAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    FName MapID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    FName RootNodeID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    TSubclassOf<AMapNode> DefaultNodeClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map", meta=(TitleProperty="Data.NodeID"))
    TArray<FMapNodeDefinition> Nodes;

    UFUNCTION(BlueprintCallable, Category="Map")
    bool ValidateDefinition(FString& Error) const;

    void GetNodeData(TArray<FMapNodeData>& OutNodes) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
