#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HeroClassesImporter.generated.h"

class UDataTable;

UCLASS()
class EGNISEDITOR_API UHeroClassesImporter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sheets Importer")
	static void SyncFromGoogleSheets();

	UFUNCTION(BlueprintCallable, Category = "Sheets Importer")
	static int32 ImportFromCSV(const FString& CSV, UDataTable* Table);
};
