#include "Cards/CardDisplayLibrary.h"
#include "Cards/BaseCard.h"
#include "Cards/CardDisplaySettings.h"
#include "Cards/Card_AreaDamageEffect.h"
#include "Cards/Card_AttackEffect.h"
#include "Cards/Card_ConsumeEffect.h"
#include "Cards/Card_CreateEffect.h"
#include "Cards/Card_DiscardEffect.h"
#include "Cards/Card_DrawEffect.h"
#include "Cards/Card_HealEffect.h"
#include "Cards/Card_KeepEffect.h"
#include "Cards/Card_MoveEffect.h"
#include "Cards/Card_PushPullEffect.h"
#include "Cards/Card_ShieldEffect.h"
#include "Cards/Card_StatusEffect.h"
#include "Cards/Card_SummonEffect.h"
#include "Cards/Card_TimeEffect.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "CardDisplay"

namespace
{
	FText StatusName(EStatusType Status)
	{
		switch (Status)
		{
		case EStatusType::Status_Burn:             return LOCTEXT("Burn", "Quemadura");
		case EStatusType::Status_Weakened:         return LOCTEXT("Weakened", "Debilidad");
		case EStatusType::Status_Exposed:          return LOCTEXT("Exposed", "Expuesto");
		case EStatusType::Status_Paralyzed:        return LOCTEXT("Paralyzed", "Parálisis");
		case EStatusType::Status_Stunned:          return LOCTEXT("Stunned", "Aturdido");
		case EStatusType::Status_Bleed:            return LOCTEXT("Bleed", "Sangrado");
		case EStatusType::Status_Taunt:            return LOCTEXT("Taunt", "Provocar");
		case EStatusType::Status_Fatigued:         return LOCTEXT("Fatigued", "Fatiga");
		case EStatusType::Status_Thorns:           return LOCTEXT("Thorns", "Espinas");
		case EStatusType::Status_FieryCurse:       return LOCTEXT("FieryCurse", "Maldición ígnea");
		case EStatusType::Status_Rooted:           return LOCTEXT("Rooted", "Enraizado");
		case EStatusType::Status_Riposte:          return LOCTEXT("Riposte", "Riposte");
		case EStatusType::Status_UnrestrainedRage: return LOCTEXT("UnrestrainedRage", "Ira irrefrenable");
		case EStatusType::Status_Slowed:           return LOCTEXT("Slowed", "Ralentizado");
		case EStatusType::Status_Ecstatic:         return LOCTEXT("Ecstatic", "Éxtasis");
		case EStatusType::Status_Elusive:          return LOCTEXT("Elusive", "Escurridizo");
		case EStatusType::Status_SpiritAttraction: return LOCTEXT("SpiritAttraction", "Atracción espiritual");
		case EStatusType::Status_Perfume:          return LOCTEXT("Perfume", "Perfume");
		case EStatusType::Status_Madness:          return LOCTEXT("Madness", "Locura");
		case EStatusType::Status_Frightened:       return LOCTEXT("Frightened", "Asustado");
		case EStatusType::Status_BoilingBlood:     return LOCTEXT("BoilingBlood", "Sangre hirviente");
		case EStatusType::Status_Ashes:            return LOCTEXT("Ashes", "Cenizas");
		default:                                   return FText::GetEmpty();
		}
	}

	bool IsBeneficialStatus(EStatusType Status)
	{
		const FCardStatusDisplay* Display = GetDefault<UEgnisCardDisplaySettings>()->Statuses.Find(Status);
		return Display && Display->bBeneficial;
	}

	int32 DamageFor(const ACharacterBase* Caster, float DamageMultiplier)
	{
		const float BaseDamage = UEgnisCombatLibrary::GetAttackPower(Caster) * DamageMultiplier;
		return FMath::RoundToInt(UEgnisCombatLibrary::CalculateDamage(Caster, nullptr, BaseDamage));
	}

	void SetValue(FCardEffectDisplay& Display, int32 Value)
	{
		Display.bHasValue = true;
		Display.Value = Value;
	}

	// Rellena icono, nombre y número según el tipo de efecto. Los efectos que no están aquí (por ejemplo
	// uno hecho solo en Blueprint) enseñan el icono de su BP, el nombre de su clase y ningún número
	void FillEffect(const UCardEffect* Effect, const ACharacterBase* Caster, FCardEffectDisplay& Display)
	{
		const AAlly* Ally = Cast<AAlly>(Caster);

		if (const UCard_AttackEffect* Attack = Cast<UCard_AttackEffect>(Effect))
		{
			Display.Label = LOCTEXT("Attack", "Daño");
			if (Caster)
			{
				SetValue(Display, DamageFor(Caster, Attack->DamageMultiplier));
			}
		}
		else if (const UCard_AreaDamageEffect* Area = Cast<UCard_AreaDamageEffect>(Effect))
		{
			Display.Label = LOCTEXT("AreaDamage", "Daño en área");
			if (Caster)
			{
				SetValue(Display, DamageFor(Caster, Area->DamageMultiplier));
			}
		}
		else if (const UCard_HealEffect* Heal = Cast<UCard_HealEffect>(Effect))
		{
			Display.Label = LOCTEXT("Heal", "Cura");
			if (Heal->PietyMultiplier <= 0.f || Ally)
			{
				SetValue(Display, FMath::RoundToInt(Heal->GetFinalAmount(Caster)));
			}
		}
		else if (const UCard_ShieldEffect* Shield = Cast<UCard_ShieldEffect>(Effect))
		{
			Display.Label = LOCTEXT("Shield", "Escudo");
			if (Shield->PietyMultiplier <= 0.f || Ally)
			{
				SetValue(Display, FMath::RoundToInt(Shield->GetFinalAmount(Caster)));
			}
		}
		else if (const UCard_DrawEffect* Draw = Cast<UCard_DrawEffect>(Effect))
		{
			Display.Label = LOCTEXT("Draw", "Robar");
			SetValue(Display, Draw->DrawAmount);
		}
		else if (const UCard_DiscardEffect* Discard = Cast<UCard_DiscardEffect>(Effect))
		{
			Display.Label = LOCTEXT("Discard", "Descartar");
			SetValue(Display, Discard->Count);
		}
		else if (const UCard_KeepEffect* Keep = Cast<UCard_KeepEffect>(Effect))
		{
			Display.Label = LOCTEXT("Keep", "Guardar");
			SetValue(Display, Keep->Count);
		}
		else if (const UCard_ConsumeEffect* Consume = Cast<UCard_ConsumeEffect>(Effect))
		{
			Display.Label = LOCTEXT("Consume", "Consumir");
			if (Consume->Mode != EConsumeMode::PlayedCard)
			{
				SetValue(Display, Consume->Count);
			}
		}
		else if (const UCard_MoveEffect* Move = Cast<UCard_MoveEffect>(Effect))
		{
			Display.Label = LOCTEXT("Move", "Mover");
			if (!Move->bUseFractionOfMovement)
			{
				SetValue(Display, Move->Distance);
			}
			else if (Caster)
			{
				SetValue(Display, Move->GetDistanceFor(Caster));
			}
		}
		else if (const UCard_PushPullEffect* PushPull = Cast<UCard_PushPullEffect>(Effect))
		{
			Display.Label = PushPull->Mode == EForcedMoveMode::Push ? LOCTEXT("Push", "Empujar") : LOCTEXT("Pull", "Tirar");
			SetValue(Display, PushPull->Distance);
		}
		else if (const UCard_StatusEffect* Status = Cast<UCard_StatusEffect>(Effect))
		{
			Display.Label = StatusName(Status->Status);
			if (const FCardStatusDisplay* StatusDisplay = GetDefault<UEgnisCardDisplaySettings>()->Statuses.Find(Status->Status))
			{
				if (UTexture2D* StatusIcon = StatusDisplay->Icon.LoadSynchronous())
				{
					Display.Icon = StatusIcon;
				}
			}
			SetValue(Display, Status->Turns);
		}
		else if (const UCard_TimeEffect* Time = Cast<UCard_TimeEffect>(Effect))
		{
			Display.Label = LOCTEXT("Time", "Tiempo");
			if (Ally)
			{
				SetValue(Display, FMath::RoundToInt(Ally->GetCharacterPiety() * Time->PietyMultiplier));
			}
		}
		else if (Effect->IsA<UCard_CreateEffect>())
		{
			Display.Label = LOCTEXT("Create", "Crear");
		}
		else if (Effect->IsA<UCard_SummonEffect>())
		{
			Display.Label = LOCTEXT("Summon", "Invocar");
		}
		else
		{
			Display.Label = Effect->GetClass()->GetDisplayNameText();
		}
	}
}

TArray<FCardEffectDisplay> UCardDisplayLibrary::GetEffectDisplays(const UBaseCard* Card, const ACharacterBase* Caster)
{
	TArray<FCardEffectDisplay> Displays;
	if (!Card)
	{
		return Displays;
	}
	for (const UCardEffect* Effect : Card->GetEffects())
	{
		if (Effect && Effect->bShowOnCard)
		{
			Displays.Add(GetEffectDisplay(Effect, Caster));
		}
	}
	if (Card->GetEnergyOnKill() > 0)
	{
		const UEgnisCardDisplaySettings* Settings = GetDefault<UEgnisCardDisplaySettings>();
		FCardEffectDisplay EnergyDisplay;
		EnergyDisplay.Icon = Settings->EnergyOnKillIcon.LoadSynchronous();
		EnergyDisplay.Label = LOCTEXT("EnergyOnKill", "Energía al matar");
		EnergyDisplay.bGoodForPlayer = true;
		EnergyDisplay.Color = Settings->GoodColor;
		SetValue(EnergyDisplay, Card->GetEnergyOnKill());
		Displays.Add(EnergyDisplay);
	}
	return Displays;
}

FCardEffectDisplay UCardDisplayLibrary::GetEffectDisplay(const UCardEffect* Effect, const ACharacterBase* Caster)
{
	FCardEffectDisplay Display;
	if (!Effect)
	{
		return Display;
	}

	Display.Icon = Effect->Icon;
	FillEffect(Effect, Caster, Display);
	if (!Effect->bShowValue)
	{
		Display.bHasValue = false;
	}

	Display.bGoodForPlayer = IsEffectGoodForPlayer(Effect);
	const UEgnisCardDisplaySettings* Settings = GetDefault<UEgnisCardDisplaySettings>();
	Display.Color = Display.bGoodForPlayer ? Settings->GoodColor : Settings->BadColor;
	return Display;
}

bool UCardDisplayLibrary::IsEffectGoodForPlayer(const UCardEffect* Effect)
{
	if (!Effect)
	{
		return true;
	}
	if (Effect->Polarity != ECardEffectPolarity::Auto)
	{
		return Effect->Polarity == ECardEffectPolarity::Good;
	}

	// Malo es lo que perjudica al jugador: perder cartas, perder tiempo o un estado que le viene mal a quien lo recibe
	if (Effect->IsA<UCard_DiscardEffect>() || Effect->IsA<UCard_ConsumeEffect>())
	{
		return false;
	}
	if (const UCard_TimeEffect* Time = Cast<UCard_TimeEffect>(Effect))
	{
		return Time->PietyMultiplier >= 0.f;
	}
	if (const UCard_StatusEffect* Status = Cast<UCard_StatusEffect>(Effect))
	{
		const bool bBeneficial = IsBeneficialStatus(Status->Status);
		if (Effect->Target == ECardTarget::Tile)
		{
			return Status->AreaFilter == EStatusAreaFilter::Enemies ? !bBeneficial : bBeneficial;
		}
		return Effect->Target == ECardTarget::Enemy ? !bBeneficial : bBeneficial;
	}
	return true;
}

ECardLaunchType UCardDisplayLibrary::GetLaunchType(const UBaseCard* Card)
{
	if (!Card)
	{
		return ECardLaunchType::None;
	}

	switch (Card->GetTarget())
	{
	case ECardTarget::Self:  return ECardLaunchType::Self;
	case ECardTarget::Ally:  return ECardLaunchType::Ally;
	case ECardTarget::Enemy: return ECardLaunchType::Enemy;
	case ECardTarget::Tile:
		for (const UCardEffect* Effect : Card->GetEffects())
		{
			if (Effect && (Effect->IsA<UCard_AreaDamageEffect>() || (Effect->IsA<UCard_StatusEffect>() && Effect->Target == ECardTarget::Tile)))
			{
				return ECardLaunchType::Area;
			}
		}
		return ECardLaunchType::Tile;
	default:                 return ECardLaunchType::None;
	}
}

UTexture2D* UCardDisplayLibrary::GetLaunchTypeIcon(ECardLaunchType LaunchType)
{
	const TSoftObjectPtr<UTexture2D>* Icon = GetDefault<UEgnisCardDisplaySettings>()->LaunchTypeIcons.Find(LaunchType);
	return Icon ? Icon->LoadSynchronous() : nullptr;
}

FText UCardDisplayLibrary::GetLaunchTypeName(ECardLaunchType LaunchType)
{
	return StaticEnum<ECardLaunchType>()->GetDisplayNameTextByValue(static_cast<int64>(LaunchType));
}

#undef LOCTEXT_NAMESPACE
