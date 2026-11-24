#include "CoreAndSystems/CombatEnvironmentSubsystem.h"

#include "Characters/Enemy.h"
#include "CoreAndSystems/EgnisEnvironmentSettings.h"
#include "CoreAndSystems/HealthAttributeSet.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Map/EventCombatSubsystem.h"
#include "TimerManager.h"
#include "CoreAndSystems/GeyserSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisEnvironment, Log, All);

bool UCombatEnvironmentSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

ECombatEnvironment UCombatEnvironmentSubsystem::ParseEnvironment(FName Name)
{
	if (Name.IsNone())
	{
		return ECombatEnvironment::None;
	}
	const FString S = Name.ToString();
	if (S.Equals(TEXT("Geiser"), ESearchCase::IgnoreCase) || S.Equals(TEXT("Geyser"), ESearchCase::IgnoreCase))
	{
		return ECombatEnvironment::Geiser;
	}
	if (S.Equals(TEXT("Fog"), ESearchCase::IgnoreCase) || S.Equals(TEXT("Niebla"), ESearchCase::IgnoreCase))
	{
		return ECombatEnvironment::Fog;
	}
	if (S.Equals(TEXT("EvilRain"), ESearchCase::IgnoreCase))
	{
		return ECombatEnvironment::EvilRain;
	}
	return ECombatEnvironment::Other;
}

void UCombatEnvironmentSubsystem::BeginDeployment()
{
	bInDeployment = true;

	const UWorld* World = GetWorld();
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UEventCombatSubsystem* EventCombat = GI ? GI->GetSubsystem<UEventCombatSubsystem>() : nullptr;
	if (EventCombat && EventCombat->IsEventCombat() && !EventCombat->GetEnvironment().IsNone())
	{
		StartEnvironment(EventCombat->GetEnvironment());
	}
}

void UCombatEnvironmentSubsystem::BeginCombat()
{
	bInDeployment = false;

	// La niebla dura mientras dura la preparacion
	if (bFogActive)
	{
		SetFogActive(false);
	}

	// Los enemigos mejoran al empezar el combate (ya existen todos y el despliegue ha terminado)
	if (Active == ECombatEnvironment::EvilRain && !bEvilRainApplied)
	{
		bEvilRainApplied = true;
		ApplyEvilRain();
	}
	
	if (Active == ECombatEnvironment::Geiser)
	{
		if (UGeyserSubsystem* Geyser = GetWorld() ? GetWorld()->GetSubsystem<UGeyserSubsystem>() : nullptr)
		{
			Geyser->BeginCombat();
		}
	}
}

void UCombatEnvironmentSubsystem::EndCombat()
{
	bInDeployment = false;
	StopEnvironment();
}

void UCombatEnvironmentSubsystem::StartEnvironment(FName Name)
{
	Active = ParseEnvironment(Name);
	ActiveName = Name;
	bEvilRainApplied = false;

	switch (Active)
	{
	case ECombatEnvironment::Fog:
		SetFogActive(true);
		break;
	case ECombatEnvironment::EvilRain:
		UE_LOG(LogEgnisEnvironment, Log, TEXT("[Entorno] EvilRain activa: los enemigos mejoran al empezar el combate."));
		break;
	case ECombatEnvironment::Geiser:
		if (UGeyserSubsystem* Geyser = GetWorld() ? GetWorld()->GetSubsystem<UGeyserSubsystem>() : nullptr)
		{
			Geyser->Setup();
		}
		break;
	case ECombatEnvironment::Other:
		UE_LOG(LogEgnisEnvironment, Warning, TEXT("[Entorno] '%s' no es un entorno con efecto. Solo se lanza el evento de Blueprint."), *Name.ToString());
		break;
	default:
		break;
	}

	OnEnvironmentStarted.Broadcast(Active, ActiveName);
}

void UCombatEnvironmentSubsystem::StopEnvironment()
{
	if (Active == ECombatEnvironment::None)
	{
		return;
	}
	if (bFogActive)
	{
		SetFogActive(false);
	}
	if (Active == ECombatEnvironment::Geiser)
	{
		if (UGeyserSubsystem* Geyser = GetWorld() ? GetWorld()->GetSubsystem<UGeyserSubsystem>() : nullptr)
		{
			Geyser->Clear();
		}
	}
	Active = ECombatEnvironment::None;
	ActiveName = NAME_None;
	bEvilRainApplied = false;
	OnEnvironmentEnded.Broadcast();
}

void UCombatEnvironmentSubsystem::SetFogActive(bool bActive)
{
	if (bFogActive == bActive)
	{
		return;
	}
	bFogActive = bActive;
	SetEnemiesHiddenByFog(bActive);

	// Si el despliegue empieza en el mismo frame que el tablero, puede que algun enemigo aun no exista: se repite en el siguiente frame
	if (bActive && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (bFogActive)
			{
				SetEnemiesHiddenByFog(true);
			}
		}));
	}

	UE_LOG(LogEgnisEnvironment, Log, TEXT("[Entorno] Niebla %s."), bActive ? TEXT("ACTIVA") : TEXT("quitada"));
	OnFogChanged.Broadcast(bActive);
}

void UCombatEnvironmentSubsystem::SetEnemiesHiddenByFog(bool bHide)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Mostrar siempre se permite, aunque el ajuste se haya cambiado a mitad
	if (bHide && !GetDefault<UEgnisEnvironmentSettings>()->bFogHidesEnemies)
	{
		return;
	}
	for (TActorIterator<AEnemy> It(World); It; ++It)
	{
		It->SetActorHiddenInGame(bHide);
	}
}

void UCombatEnvironmentSubsystem::ApplyEvilRain()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const UEgnisEnvironmentSettings* Settings = GetDefault<UEgnisEnvironmentSettings>();

	int32 Count = 0;
	for (TActorIterator<AEnemy> It(World); It; ++It)
	{
		AEnemy* Enemy = *It;

		// Vida: se escala la maxima y la actual (redondeo hacia arriba, igual que las stats de la tabla)
		if (UHealthAttributeSet* Health = Enemy->HealthAttributeSet)
		{
			if (!FMath::IsNearlyEqual(Settings->EvilRainEnemyHealthMultiplier, 1.f))
			{
				const float NewMax = FMath::CeilToFloat(Health->GetMaxHealth() * Settings->EvilRainEnemyHealthMultiplier - KINDA_SMALL_NUMBER);
				const float NewCurrent = FMath::CeilToFloat(Health->GetHealth() * Settings->EvilRainEnemyHealthMultiplier - KINDA_SMALL_NUMBER);
				Health->InitMaxHealth(NewMax);
				Health->InitHealth(FMath::Min(NewCurrent, NewMax));
			}
		}

		Enemy->EnvDamageMultiplier = Settings->EvilRainEnemyDamageMultiplier;
		Enemy->EnvMoveRangeBonus = Settings->EvilRainEnemyMoveRangeBonus;
		++Count;

		UE_LOG(LogEgnisEnvironment, Log, TEXT("[Entorno] EvilRain: %s vida %.0f/%.0f, dano x%.2f, movimiento +%d"),
			*Enemy->GetName(), Enemy->GetCurrentHealth(), Enemy->GetMaxHealth(), Enemy->EnvDamageMultiplier, Enemy->EnvMoveRangeBonus);
	}
	UE_LOG(LogEgnisEnvironment, Log, TEXT("[Entorno] EvilRain aplicada a %d enemigo(s)."), Count);
}

bool UCombatEnvironmentSubsystem::DebugSetEnvironment(FName Name)
{
	if (!bInDeployment)
	{
		UE_LOG(LogEgnisEnvironment, Warning, TEXT("[Entorno] Solo se puede poner un entorno durante el despliegue."));
		return false;
	}
	StopEnvironment();
	StartEnvironment(Name);
	return true;
}

void UCombatEnvironmentSubsystem::DebugClear()
{
	StopEnvironment();
}

void UCombatEnvironmentSubsystem::DumpToLog() const
{
	UE_LOG(LogEgnisEnvironment, Log, TEXT("[Entorno] activo=%s (%s), niebla=%d, despliegue=%d, lluvia aplicada=%d"),
		*UEnum::GetValueAsString(Active), *ActiveName.ToString(), bFogActive, bInDeployment, bEvilRainApplied);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AEnemy> It(World); It; ++It)
	{
		const AEnemy* Enemy = *It;
		UE_LOG(LogEgnisEnvironment, Log, TEXT("  %s: vida %.0f/%.0f, dano x%.2f, movimiento %d (bonus +%d), oculto=%d"),
			*Enemy->GetName(), Enemy->GetCurrentHealth(), Enemy->GetMaxHealth(), Enemy->GetDamageMultiplier(),
			Enemy->GetMoveRange(), Enemy->EnvMoveRangeBonus, Enemy->IsHidden());
	}
}

static UCombatEnvironmentSubsystem* GetEnvironmentSubsystem(UWorld* World)
{
	return World ? World->GetSubsystem<UCombatEnvironmentSubsystem>() : nullptr;
}

static FAutoConsoleCommandWithWorldAndArgs GEgnisEnvSetCmd(
	TEXT("Egnis.Env.Set"),
	TEXT("Pone un entorno en el combate, solo durante el despliegue: Egnis.Env.Set Geiser|Fog|EvilRain"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogEgnisEnvironment, Warning, TEXT("Uso: Egnis.Env.Set <Geiser|Fog|EvilRain>"));
			return;
		}
		if (UCombatEnvironmentSubsystem* Sub = GetEnvironmentSubsystem(World))
		{
			Sub->DebugSetEnvironment(FName(*Args[0]));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GEgnisEnvClearCmd(
	TEXT("Egnis.Env.Clear"),
	TEXT("Quita el entorno actual (la niebla se retira y los enemigos reaparecen)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		if (UCombatEnvironmentSubsystem* Sub = GetEnvironmentSubsystem(World))
		{
			Sub->DebugClear();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GEgnisEnvDumpCmd(
	TEXT("Egnis.Env.Dump"),
	TEXT("Muestra el entorno activo y el estado de cada enemigo (vida, dano, movimiento, oculto)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		if (UCombatEnvironmentSubsystem* Sub = GetEnvironmentSubsystem(World))
		{
			Sub->DumpToLog();
		}
	}));