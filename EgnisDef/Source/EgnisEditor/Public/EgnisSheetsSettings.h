#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "EgnisSheetsSettings.generated.h"

UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Sheets Importer"))
class EGNISEDITOR_API UEgnisSheetsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEgnisSheetsSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Clases")
	FString HeroClassesCSVUrl;

	UPROPERTY(Config, EditAnywhere, Category = "Clases")
	TSoftObjectPtr<UDataTable> HeroClassesTable;

	UPROPERTY(Config, EditAnywhere, Category = "Clases")
	FString ClassStatsBlockTitle = TEXT("CLASS Stats");

	UPROPERTY(Config, EditAnywhere, Category = "Clases")
	TMap<FString, FName> HeroClassRowNames;
};
