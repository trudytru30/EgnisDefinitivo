#include "Progression/RunModifierSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/CharacterStatsAttributesSet.h"
#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisModifiers, Log, All);

namespace
{
	FGameplayAttribute ToAttribute(ERunModifierStat Stat)
	{
		switch (Stat)
		{
		case ERunModifierStat::Power: return UCharacterStatsAttributeSet::GetPowerAttribute();
		case ERunModifierStat::Piety: return UCharacterStatsAttributeSet::GetPietyAttribute();
		case ERunModifierStat::Speed: return UCharacterStatsAttributeSet::GetSpeedAttribute();
		default:                      return UCharacterStatsAttributeSet::GetRangeAttribute();
		}
	}
}

void URunModifierSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (URunProgressionSubsystem* Run = Collection.InitializeDependency<URunProgressionSubsystem>())
	{
		Run->OnRunChanged.AddDynamic(this, &URunModifierSubsystem::HandleRunChanged);
	}
}

void URunModifierSubsystem::HandleRunChanged()
{
	// Al cerrarse la run se pierden los modificadores pendientes
	const UGameInstance* GI = GetGameInstance();
	const URunProgressionSubsystem* Run = GI ? GI->GetSubsystem<URunProgressionSubsystem>() : nullptr;
	if (Run && !Run->IsRunActive() && Modifiers.Num() > 0)
	{
		ClearAll();
	}
}

void URunModifierSubsystem::AddModifier(const FRunModifier& Modifier)
{
	const bool bHasEffect = Modifier.StatDeltas.Num() > 0
		|| !FMath::IsNearlyEqual(Modifier.DamageDealtMultiplier, 1.f)
		|| !FMath::IsNearlyEqual(Modifier.DamageTakenMultiplier, 1.f);
	if (!bHasEffect || Modifier.CombatsLeft <= 0)
	{
		return;
	}
	Modifiers.Add(Modifier);
	UE_LOG(LogEgnisModifiers, Log, TEXT("[Modificador] anadido: heroe '%s', culto '%s', %d stats, hecho x%.2f, recibido x%.2f, %d combates."),
		*Modifier.HeroId.ToString(), *Modifier.Cult.ToString(), Modifier.StatDeltas.Num(),
		Modifier.DamageDealtMultiplier, Modifier.DamageTakenMultiplier, Modifier.CombatsLeft);
	OnModifiersChanged.Broadcast();
}

void URunModifierSubsystem::ClearAll()
{
	Modifiers.Reset();
	CombatMultipliers.Reset();
	OnModifiersChanged.Broadcast();
}

void URunModifierSubsystem::ApplyToAlly(AAlly* Ally, FName HeroId)
{
	UAbilitySystemComponent* ASC = Ally ? Ally->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	FVector2D Multipliers(1.0, 1.0);
	for (const FRunModifier& Mod : Modifiers)
	{
		if (!Mod.HeroId.IsNone() && Mod.HeroId != HeroId) continue;
		if (Mod.Cult.IsValid() && !Ally->HasCult(Mod.Cult)) continue;

		for (const TPair<ERunModifierStat, float>& Delta : Mod.StatDeltas)
		{
			const FGameplayAttribute Attribute = ToAttribute(Delta.Key);
			const float Before = ASC->GetNumericAttributeBase(Attribute);
			ASC->SetNumericAttributeBase(Attribute, Before + Delta.Value);
			UE_LOG(LogEgnisModifiers, Log, TEXT("[Modificador] %s: %s %.1f -> %.1f"),
				*HeroId.ToString(), *Attribute.GetName(), Before, Before + Delta.Value);
		}
		Multipliers.X *= Mod.DamageDealtMultiplier;
		Multipliers.Y *= Mod.DamageTakenMultiplier;
	}

	if (!FMath::IsNearlyEqual(Multipliers.X, 1.0) || !FMath::IsNearlyEqual(Multipliers.Y, 1.0))
	{
		CombatMultipliers.Add(TWeakObjectPtr<const ACharacterBase>(Ally), Multipliers);
		UE_LOG(LogEgnisModifiers, Log, TEXT("[Modificador] %s: dano hecho x%.2f, dano recibido x%.2f"),
			*HeroId.ToString(), Multipliers.X, Multipliers.Y);
	}
}

void URunModifierSubsystem::ConsumeCombat()
{
	CombatMultipliers.Reset();
	if (Modifiers.Num() == 0)
	{
		return;
	}

	for (int32 Index = Modifiers.Num() - 1; Index >= 0; --Index)
	{
		if (--Modifiers[Index].CombatsLeft <= 0)
		{
			Modifiers.RemoveAt(Index);
		}
	}
	UE_LOG(LogEgnisModifiers, Log, TEXT("[Modificador] combate gastado, quedan %d modificadores."), Modifiers.Num());
	OnModifiersChanged.Broadcast();
}

float URunModifierSubsystem::GetDamageDealtMultiplier(const ACharacterBase* Character) const
{
	const FVector2D* Found = Character ? CombatMultipliers.Find(TWeakObjectPtr<const ACharacterBase>(Character)) : nullptr;
	return Found ? Found->X : 1.f;
}

float URunModifierSubsystem::GetDamageTakenMultiplier(const ACharacterBase* Character) const
{
	const FVector2D* Found = Character ? CombatMultipliers.Find(TWeakObjectPtr<const ACharacterBase>(Character)) : nullptr;
	return Found ? Found->Y : 1.f;
}

void URunModifierSubsystem::DumpToLog() const
{
	UE_LOG(LogEgnisModifiers, Log, TEXT("[Modificador] %d activos:"), Modifiers.Num());
	for (const FRunModifier& Mod : Modifiers)
	{
		FString Stats;
		for (const TPair<ERunModifierStat, float>& Delta : Mod.StatDeltas)
		{
			Stats += FString::Printf(TEXT("%s%+.1f "), *UEnum::GetValueAsString(Delta.Key), Delta.Value);
		}
		UE_LOG(LogEgnisModifiers, Log, TEXT("  heroe '%s' culto '%s' | %s| hecho x%.2f recibido x%.2f | combates %d"),
			*Mod.HeroId.ToString(), *Mod.Cult.ToString(), *Stats, Mod.DamageDealtMultiplier, Mod.DamageTakenMultiplier, Mod.CombatsLeft);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GEgnisModDumpCmd(
	TEXT("Egnis.Mod.Dump"),
	TEXT("Muestra en el log los modificadores de la run"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		if (URunModifierSubsystem* Mods = GI ? GI->GetSubsystem<URunModifierSubsystem>() : nullptr)
		{
			Mods->DumpToLog();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GEgnisModClearCmd(
	TEXT("Egnis.Mod.Clear"),
	TEXT("Borra todos los modificadores de la run"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		if (URunModifierSubsystem* Mods = GI ? GI->GetSubsystem<URunModifierSubsystem>() : nullptr)
		{
			Mods->ClearAll();
		}
	}));