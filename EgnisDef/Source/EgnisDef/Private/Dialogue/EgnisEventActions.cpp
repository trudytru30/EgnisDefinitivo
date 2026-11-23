#include "Dialogue/EgnisEventActions.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Cards/BaseCard.h"
#include "Characters/HeroRuntimeData.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Map/EventCombatSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Progression/RunModifierSubsystem.h"
#include "Progression/RunState.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisEventActions, Log, All);

namespace
{
	// Héroe al azar entre los desbloqueados de la run (None si no hay ninguno que cumpla el filtro)
	FName PickRandomHero(const URunProgressionSubsystem* Run, bool bOnlyWithWounds, bool bOnlyBelowMaxWounds)
	{
		TArray<FName> Candidates;
		for (const FHeroRuntimeData& Hero : Run->GetUnlockedHeroes())
		{
			if (bOnlyWithWounds && Hero.Wounds.Num() == 0) continue;
			if (bOnlyBelowMaxWounds && Hero.Wounds.Num() >= EgnisRun::MaxWoundsPerHero) continue;
			Candidates.Add(Hero.HeroId);
		}
		return Candidates.Num() > 0 ? Candidates[FMath::RandRange(0, Candidates.Num() - 1)] : NAME_None;
	}

	// Lee "Power=2", "DamageDealt=0.5"... y lo mete en el modificador. Devuelve false si no reconoce la clave.
	bool ParseModifierValue(const FString& Key, float Value, FRunModifier& Mod)
	{
		if (Key.Equals(TEXT("Power"), ESearchCase::IgnoreCase))       { Mod.StatDeltas.Add(ERunModifierStat::Power, Value); return true; }
		if (Key.Equals(TEXT("Piety"), ESearchCase::IgnoreCase))       { Mod.StatDeltas.Add(ERunModifierStat::Piety, Value); return true; }
		if (Key.Equals(TEXT("Speed"), ESearchCase::IgnoreCase))       { Mod.StatDeltas.Add(ERunModifierStat::Speed, Value); return true; }
		if (Key.Equals(TEXT("Range"), ESearchCase::IgnoreCase))       { Mod.StatDeltas.Add(ERunModifierStat::Range, Value); return true; }
		if (Key.Equals(TEXT("DamageDealt"), ESearchCase::IgnoreCase)) { Mod.DamageDealtMultiplier = Value; return true; }
		if (Key.Equals(TEXT("DamageTaken"), ESearchCase::IgnoreCase)) { Mod.DamageTakenMultiplier = Value; return true; }
		return false;
	}

	// Cartas de color (sin las versiones Grey, que son el ConvertedCard de otra carta)
	void GetAllRewardCards(TArray<UBaseCard*>& OutCards)
	{
		FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		Module.Get().GetAssetsByClass(UBaseCard::StaticClass()->GetClassPathName(), Assets, true);

		TArray<UBaseCard*> All;
		TSet<const UBaseCard*> Converted;
		for (const FAssetData& Data : Assets)
		{
			if (UBaseCard* Card = Cast<UBaseCard>(Data.GetAsset()))
			{
				All.Add(Card);
				if (const UBaseCard* Grey = Card->GetConvertedCard())
				{
					Converted.Add(Grey);
				}
			}
		}
		for (UBaseCard* Card : All)
		{
			if (!Converted.Contains(Card))
			{
				OutCards.Add(Card);
			}
		}
	}
}

bool UEgnisEventActions::ExecuteAction(UObject* WorldContextObject, const FString& Action)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	URunProgressionSubsystem* Run = GI ? GI->GetSubsystem<URunProgressionSubsystem>() : nullptr;
	if (!Run)
	{
		UE_LOG(LogEgnisEventActions, Error, TEXT("Accion '%s': no hay subsistema de run."), *Action);
		return false;
	}

	TArray<FString> Tokens;
	Action.TrimStartAndEnd().ParseIntoArray(Tokens, TEXT(" "), true);
	if (Tokens.Num() == 0)
	{
		return false;
	}

	const FString& Command = Tokens[0];
	const FString Arg = Tokens.Num() > 1 ? Tokens[1] : FString();
	const int32 Amount = Arg.IsEmpty() ? 1 : FCString::Atoi(*Arg);

	if (Command.Equals(TEXT("AddBones"), ESearchCase::IgnoreCase))
	{
		Run->AddBones(Amount);
	}
	else if (Command.Equals(TEXT("AddRandomWound"), ESearchCase::IgnoreCase))
	{
		const FName Hero = PickRandomHero(Run, false, true);
		if (!Hero.IsNone())
		{
			Run->AddRandomWound(Hero);
		}
	}
	else if (Command.Equals(TEXT("HealWound"), ESearchCase::IgnoreCase))
	{
		const FName Hero = PickRandomHero(Run, true, false);
		if (!Hero.IsNone())
		{
			Run->HealOneWound(Hero);
		}
	}
	else if (Command.Equals(TEXT("RestoreCampfire"), ESearchCase::IgnoreCase))
	{
		Run->RestoreCampfire(Amount);
	}
	else if (Command.Equals(TEXT("LoseCampfire"), ESearchCase::IgnoreCase))
	{
		Run->LoseCampfire();
	}
	else if (Command.Equals(TEXT("AddRelic"), ESearchCase::IgnoreCase) && !Arg.IsEmpty())
	{
		Run->AddRelic(FName(*Arg));
	}
	else if (Command.Equals(TEXT("RemoveRandomCard"), ESearchCase::IgnoreCase))
	{
		const int32 NumCards = Run->GetDeckCards().Num();
		if (NumCards > 0)
		{
			Run->RemoveCardFromDeck(FMath::RandRange(0, NumCards - 1));
		}
	}
	else if (Command.Equals(TEXT("AddRandomCard"), ESearchCase::IgnoreCase))
	{
		// Provisional (diseño): sale de todas las cartas del juego. Luego se podrá filtrar por culto o rareza.
		TArray<UBaseCard*> Pool;
		GetAllRewardCards(Pool);
		if (Pool.Num() == 0)
		{
			UE_LOG(LogEgnisEventActions, Warning, TEXT("AddRandomCard: no se ha encontrado ninguna carta."));
			return false;
		}
		for (int32 Index = 0; Index < FMath::Max(1, Amount); ++Index)
		{
			Run->AddCardToDeck(Pool[FMath::RandRange(0, Pool.Num() - 1)]);
		}
	}
	else if (Command.Equals(TEXT("UnlockHero"), ESearchCase::IgnoreCase) && !Arg.IsEmpty())
	{
		Run->UnlockHero(FName(*Arg));
	}
	else if (Command.Equals(TEXT("Modifier"), ESearchCase::IgnoreCase) && Tokens.Num() >= 3)
	{
		// Modifier <Objetivo> <Clave=Valor> [<Clave=Valor>...] x<Combates>
		// Objetivo: all | Egnis | Doru | Mern | Wers | random | <HeroId>
		// Claves: Power, Piety, Speed, Range (suman), DamageDealt, DamageTaken (multiplican: 0.5 = mitad, 2 = doble)
		URunModifierSubsystem* Mods = GI->GetSubsystem<URunModifierSubsystem>();
		if (!Mods)
		{
			return false;
		}

		FRunModifier Mod;
		const FString& Target = Tokens[1];
		if (Target.Equals(TEXT("Egnis"), ESearchCase::IgnoreCase))      { Mod.Cult = TAG_Cult_Egnis; }
		else if (Target.Equals(TEXT("Doru"), ESearchCase::IgnoreCase))  { Mod.Cult = TAG_Cult_Doru; }
		else if (Target.Equals(TEXT("Mern"), ESearchCase::IgnoreCase) || Target.Equals(TEXT("Mérn"), ESearchCase::IgnoreCase)) { Mod.Cult = TAG_Cult_Mern; }
		else if (Target.Equals(TEXT("Wers"), ESearchCase::IgnoreCase))  { Mod.Cult = TAG_Cult_Wers; }
		else if (Target.Equals(TEXT("random"), ESearchCase::IgnoreCase))
		{
			Mod.HeroId = PickRandomHero(Run, false, false);
			if (Mod.HeroId.IsNone())
			{
				return false;
			}
		}
		else if (!Target.Equals(TEXT("all"), ESearchCase::IgnoreCase))
		{
			Mod.HeroId = FName(*Target);
		}

		bool bAnyEffect = false;
		for (int32 Index = 2; Index < Tokens.Num(); ++Index)
		{
			FString Key, Value;
			if (Tokens[Index].Split(TEXT("="), &Key, &Value))
			{
				bAnyEffect |= ParseModifierValue(Key, FCString::Atof(*Value), Mod);
			}
			else if (Tokens[Index].StartsWith(TEXT("x"), ESearchCase::IgnoreCase))
			{
				Mod.CombatsLeft = FMath::Max(1, FCString::Atoi(*Tokens[Index].Mid(1)));
			}
		}

		if (!bAnyEffect)
		{
			UE_LOG(LogEgnisEventActions, Warning, TEXT("Modifier sin ningun efecto valido: '%s'"), *Action);
			return false;
		}
		Mods->AddModifier(Mod);
	}
	else if (Command.Equals(TEXT("StartCombat"), ESearchCase::IgnoreCase)
		|| Command.Equals(TEXT("OnWin"), ESearchCase::IgnoreCase)
		|| Command.Equals(TEXT("OnLose"), ESearchCase::IgnoreCase)
		|| Command.Equals(TEXT("CombatEnv"), ESearchCase::IgnoreCase))
	{
		// StartCombat [ClaveDeNivel]    -> al acabar el dialogo se abre un combate (nivel por defecto si no hay clave)
		// OnWin <accion>                -> accion que se ejecuta si se gana ese combate (puede haber varias)
		// OnLose <accion>               -> accion que se ejecuta si se pierde
		// CombatEnv <Nombre>            -> entorno del combate (Geiser, Fog, EvilRain...); el combate lo lee con GetEnvironment()
		UEventCombatSubsystem* EventCombat = GI->GetSubsystem<UEventCombatSubsystem>();
		if (!EventCombat)
		{
			return false;
		}

		if (Command.Equals(TEXT("StartCombat"), ESearchCase::IgnoreCase))
		{
			EventCombat->RequestCombat(Arg.IsEmpty() ? NAME_None : FName(*Arg));
		}
		else if (Command.Equals(TEXT("CombatEnv"), ESearchCase::IgnoreCase))
		{
			if (Arg.IsEmpty())
			{
				return false;
			}
			EventCombat->SetEnvironment(FName(*Arg));
		}
		else
		{
			// Lo que va despues de OnWin / OnLose es una accion completa, tal cual
			const FString Inner = Action.TrimStartAndEnd().Mid(Command.Len()).TrimStartAndEnd();
			if (Inner.IsEmpty())
			{
				return false;
			}
			if (Command.Equals(TEXT("OnWin"), ESearchCase::IgnoreCase))
			{
				EventCombat->AddWinAction(Inner);
			}
			else
			{
				EventCombat->AddLoseAction(Inner);
			}
		}
	}
	else
	{
		UE_LOG(LogEgnisEventActions, Log, TEXT("Accion '%s' sin implementar (se ignora)."), *Action);
		return false;
	}

	UE_LOG(LogEgnisEventActions, Log, TEXT("Accion ejecutada: %s"), *Action);
	return true;
}

// Para probar sin tocar el Sheet: Egnis.Event.Action AddBones 3
static FAutoConsoleCommandWithWorldAndArgs GEgnisEventActionCmd(
	TEXT("Egnis.Event.Action"),
	TEXT("Ejecuta una accion de evento sobre la run. Ej: Egnis.Event.Action AddBones 3"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UEgnisEventActions::ExecuteAction(World, FString::Join(Args, TEXT(" ")));
	}));