#include "Characters/HeroDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UHeroDataAsset::UHeroDataAsset()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> DefaultDataTable(TEXT("/Game/CharactersData/Player/Classes/DT_HeroClasses.DT_HeroClasses"));
	if (DefaultDataTable.Succeeded())
	{
		HeroStatsRow.DataTable = DefaultDataTable.Object;
	}
}