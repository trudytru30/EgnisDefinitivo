#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/ColorType.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Damage, "Data.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Burn, "Status.Burn");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Weakened, "Status.Weakened");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Exposed, "Status.Exposed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Paralyzed, "Status.Paralyzed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Stunned, "Status.Stunned");

UE_DEFINE_GAMEPLAY_TAG(TAG_Cult, "Cult");
UE_DEFINE_GAMEPLAY_TAG(TAG_Cult_Doru, "Cult.Doru");
UE_DEFINE_GAMEPLAY_TAG(TAG_Cult_Egnis, "Cult.Egnis");
UE_DEFINE_GAMEPLAY_TAG(TAG_Cult_Mern, TEXT("Cult.Mérn"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Cult_Wers, "Cult.Wers");

namespace EgnisCults
{
	FGameplayTag FromLegacyColor(EColorType Color)
	{
		switch (Color)
		{
		case EColorType::Turquoise:   return TAG_Cult_Doru;
		case EColorType::Indigo:      return TAG_Cult_Mern;
		case EColorType::DarkMagenta: return TAG_Cult_Egnis;
		case EColorType::Vermillion:  return TAG_Cult_Wers;
		default:                      return FGameplayTag();
		}
	}

	EColorType ToLegacyColor(const FGameplayTag& Cult)
	{
		if (Cult == TAG_Cult_Doru)  return EColorType::Turquoise;
		if (Cult == TAG_Cult_Mern)  return EColorType::Indigo;
		if (Cult == TAG_Cult_Egnis) return EColorType::DarkMagenta;
		if (Cult == TAG_Cult_Wers)  return EColorType::Vermillion;
		return EColorType::Grey;
	}
}
