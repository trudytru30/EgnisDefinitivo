#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapEncounterExit.generated.h"

// Prototype encounter: Space completes the current node and returns to its map.
UCLASS(Blueprintable)
class EGNISDEF_API AMapEncounterExit : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map")
    TSoftObjectPtr<UWorld> ReturnMap;

    UFUNCTION(BlueprintCallable, Category="Map")
    bool CompleteAndReturn();

protected:
    virtual void BeginPlay() override;
    void HandleComplete();
};

