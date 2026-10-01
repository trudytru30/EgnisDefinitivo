#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "Characters/HeroDataAsset.h"

void URunProgressionSubsystem::InitializeRunRoster(const TArray<UHeroDataAsset*>& AllHeroAssets)
{
	PlayerRoster.Empty();

	for (UHeroDataAsset* Asset : AllHeroAssets)
	{
		if (!Asset) continue;

		FHeroRuntimeData RuntimeData;
		RuntimeData.HeroDataAsset = Asset;
        
		RuntimeData.bIsUnlocked = (PlayerRoster.Num() == 0); 
		RuntimeData.CurrentHealth = 100.f;

		PlayerRoster.Add(Asset->ExcelRowName, RuntimeData);
	}
}

TArray<FHeroRuntimeData> URunProgressionSubsystem::GetUnlockedHeroes() const
{
	TArray<FHeroRuntimeData> Result;
	for (const auto& Pair : PlayerRoster)
	{
		if (Pair.Value.bIsUnlocked)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

void URunProgressionSubsystem::UnlockHero(FName ExcelRowName)
{
	if (FHeroRuntimeData* FoundHero = PlayerRoster.Find(ExcelRowName))
	{
		FoundHero->bIsUnlocked = true;
	}
}

void URunProgressionSubsystem::UpdateHeroHealth(FName ExcelRowName, float NewHealth)
{
	if (FHeroRuntimeData* FoundHero = PlayerRoster.Find(ExcelRowName))
	{
		FoundHero->CurrentHealth = NewHealth;
	}
}