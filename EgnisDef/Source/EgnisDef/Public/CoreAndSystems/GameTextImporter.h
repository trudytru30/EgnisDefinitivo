#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/DataTable.h"
#include "GameTextImporter.generated.h"

UCLASS()
class EGNISDEF_API UGameTextImporter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Localization|Import")
	static void SyncDataTableFromGoogleSheets(const FString& CSVUrl, UDataTable* TargetDataTable);
};
