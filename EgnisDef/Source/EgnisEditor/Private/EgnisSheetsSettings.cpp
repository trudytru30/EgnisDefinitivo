#include "EgnisSheetsSettings.h"

UEgnisSheetsSettings::UEgnisSheetsSettings()
{
	CategoryName = TEXT("Egnis");
	SectionName = TEXT("Sheets Importer");

	HeroClassesCSVUrl = TEXT("https://docs.google.com/spreadsheets/d/e/2PACX-1vT6VsNIutM-IChd4VFVwYt--rk3nXs7LgaB-qqsSaD03n0gni68csJZXM6ueWCS_0oVeuCd3cYIgPYl/pub?gid=0&single=true&output=csv");
	HeroClassesTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/DataAssets/Characters/Player/Classes/DT_HeroClasses.DT_HeroClasses")));

	HeroClassRowNames.Add(TEXT("TAKEN"), TEXT("Class_Arrebatado"));
	HeroClassRowNames.Add(TEXT("IGNIFEROUS"), TEXT("Class_Ignifero"));
	HeroClassRowNames.Add(TEXT("STAINED"), TEXT("Class_Tizonado"));
	HeroClassRowNames.Add(TEXT("RUPESTRIAN"), TEXT("Class_Rupestre"));
	HeroClassRowNames.Add(TEXT("YOKE-BEARER"), TEXT("Class_Uncidor"));
	HeroClassRowNames.Add(TEXT("SEEDER"), TEXT("Class_Sembrador"));
	HeroClassRowNames.Add(TEXT("APOTHECARY"), TEXT("Class_Apotecario"));
	HeroClassRowNames.Add(TEXT("RUINICOLOUS"), TEXT("Class_Ruinicola"));
	HeroClassRowNames.Add(TEXT("RELIQUARY"), TEXT("Class_Relicario"));
}
