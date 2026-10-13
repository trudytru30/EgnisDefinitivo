#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/MapTypes.h"
#include "MapNode.generated.h"

class USphereComponent;
class ALevelMapManager;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class EGNISDEF_API AMapNode : public AActor
{
    GENERATED_BODY()

public:
    AMapNode();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Map")
    TObjectPtr<USphereComponent> ClickTarget;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    FMapNodeData Data;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Map")
    EMapNodeState CurrentState = EMapNodeState::Locked;

    UFUNCTION(BlueprintCallable, Category="Map")
    bool TryEnterNode();

    UFUNCTION(BlueprintNativeEvent, Category="Map")
    void RefreshVisualState(EMapNodeState NewState);

    virtual void RefreshVisualState_Implementation(EMapNodeState NewState);
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    void SetManager(ALevelMapManager* NewManager);
    void UpdateState(EMapNodeState NewState);

private:
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> NodeMaterial;

    UPROPERTY()
    TObjectPtr<ALevelMapManager> Manager;

    UFUNCTION()
    void HandleClicked(UPrimitiveComponent* Component, FKey Button);
};

