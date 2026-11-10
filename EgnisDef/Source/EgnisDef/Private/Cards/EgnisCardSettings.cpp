#include "Cards/EgnisCardSettings.h"
#include "CoreAndSystems/EgnisGameplayTags.h"

UEgnisCardSettings::UEgnisCardSettings()
{
	CategoryName = TEXT("Egnis");
	SectionName = TEXT("Cartas");

	CultColors.Add(TAG_Cult_Doru,  FLinearColor(0.25f, 0.75f, 0.70f, 1.f));
	CultColors.Add(TAG_Cult_Mern,  FLinearColor(0.25f, 0.00f, 0.51f, 1.f));
	CultColors.Add(TAG_Cult_Egnis, FLinearColor(0.55f, 0.00f, 0.35f, 1.f));
	CultColors.Add(TAG_Cult_Wers,  FLinearColor(0.791298f, 0.027321f, 0.00f, 1.f));
}

FLinearColor UEgnisCardSettings::GetCultColor(FGameplayTag Cult)
{
	const UEgnisCardSettings* Settings = GetDefault<UEgnisCardSettings>();
	if (const FLinearColor* Found = Settings->CultColors.Find(Cult))
	{
		return *Found;
	}
	return Settings->CommonCardColor;
}
