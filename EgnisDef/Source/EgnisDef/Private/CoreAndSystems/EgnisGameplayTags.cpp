#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/ColorType.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Damage, "Data.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Burn, "Status.Burn");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Weakened, "Status.Weakened");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Exposed, "Status.Exposed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Paralyzed, "Status.Paralyzed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Stunned, "Status.Stunned");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Bleed, "Status.Bleed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Taunt, "Status.Taunt");

UE_DEFINE_GAMEPLAY_TAG(TAG_Cult, "Cult");
UE_DEFINE_GAMEPLAY_TAG(TAG_Cult_Doru, "Cult.Doru");
UE_DEFINE_GAMEPLAY_TAG(TAG_Cult_Egnis, "Cult.Egnis");
UE_DEFINE_GAMEPLAY_TAG(TAG_Cult_Mern, TEXT("Cult.Mérn"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Cult_Wers, "Cult.Wers");

UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Turn_Start, "Event.Turn.Start");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Turn_End, "Event.Turn.End");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Card_Played, "Event.Card.Played");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Damage_Taken, "Event.Damage.Taken");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Damage_Dealt, "Event.Damage.Dealt");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Unit_Killed, "Event.Unit.Killed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Unit_Died, "Event.Unit.Died");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Unit_Moved, "Event.Unit.Moved");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Summon_Created, "Event.Summon.Created");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Obstacle_Created, "Event.Obstacle.Created");

UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Class, "Ability.Class");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Unique, "Ability.Unique");
UE_DEFINE_GAMEPLAY_TAG(TAG_Rule, "Rule");
UE_DEFINE_GAMEPLAY_TAG(TAG_Immunity, "Immunity");

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
