#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MapBlueprintMigrationLibrary.generated.h"
class UBlueprint;
class UWorld;

// Editor-only helpers: preserve components while replacing legacy executable graphs.
UCLASS()
class EGNISDEFEDITOR_API UMapBlueprintMigrationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Map|Migration")
    static void ClearLegacyLogic(UBlueprint* Blueprint, bool bRemoveVariables, bool bKeepConstruction);

    UFUNCTION(BlueprintCallable, Category="Map|Migration")
    static void RepairComponentParents(UBlueprint* Blueprint);

    UFUNCTION(BlueprintCallable, Category="Map|Migration")
    static void ClearLevelLogic(UWorld* World);

    UFUNCTION(BlueprintCallable, Category="Map|Migration")
    static bool IsBlueprintValid(UBlueprint* Blueprint);

    UFUNCTION(BlueprintCallable, Category="Map|Migration")
    static FString ExportGraphSummary(UBlueprint* Blueprint);
};

