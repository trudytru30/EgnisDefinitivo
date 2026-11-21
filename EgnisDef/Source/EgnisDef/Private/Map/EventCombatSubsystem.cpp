#include "Map/EventCombatSubsystem.h"

#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "Dialogue/EgnisEventActions.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Map/MapProgressionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisEventCombat, Log, All);

void UEventCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (URunProgressionSubsystem* Run = Collection.InitializeDependency<URunProgressionSubsystem>())
	{
		Run->OnRunChanged.AddDynamic(this, &UEventCombatSubsystem::HandleRunChanged);
	}
	Collection.InitializeDependency<UMapProgressionSubsystem>();
}

void UEventCombatSubsystem::HandleRunChanged()
{
	// Al cerrarse la run se descarta cualquier combate de evento pendiente
	const UGameInstance* GI = GetGameInstance();
	const URunProgressionSubsystem* Run = GI ? GI->GetSubsystem<URunProgressionSubsystem>() : nullptr;
	if (Run && !Run->IsRunActive() && (bRequested || bInCombat))
	{
		ClearAll();
	}
}

void UEventCombatSubsystem::ClearAll()
{
	bRequested = false;
	bInCombat = false;
	LevelKey = NAME_None;
	Environment = NAME_None;
	CombatNodeID = NAME_None;
	WinActions.Reset();
	LoseActions.Reset();
}

void UEventCombatSubsystem::RequestCombat(FName InLevelKey)
{
	bRequested = true;
	LevelKey = InLevelKey;
	UE_LOG(LogEgnisEventCombat, Log, TEXT("Combate de evento pedido (nivel '%s')."), *InLevelKey.ToString());
}

void UEventCombatSubsystem::AddWinAction(const FString& Action)
{
	WinActions.Add(Action);
}

void UEventCombatSubsystem::AddLoseAction(const FString& Action)
{
	LoseActions.Add(Action);
}

void UEventCombatSubsystem::SetEnvironment(FName InEnvironment)
{
	Environment = InEnvironment;
	UE_LOG(LogEgnisEventCombat, Log, TEXT("Entorno del combate: %s"), *InEnvironment.ToString());
}

bool UEventCombatSubsystem::ConsumeCombatRequest(FName NodeID, FName& OutLevelKey)
{
	if (!bRequested)
	{
		// El dialogo acabo sin combate (por ejemplo, el jugador eligio irse): lo preparado no vale
		if (WinActions.Num() > 0 || LoseActions.Num() > 0 || !Environment.IsNone())
		{
			UE_LOG(LogEgnisEventCombat, Log, TEXT("El evento acabo sin combate: se descartan OnWin/OnLose/CombatEnv preparados."));
		}
		ClearAll();
		return false;
	}

	OutLevelKey = LevelKey;
	bRequested = false;
	bInCombat = true;
	CombatNodeID = NodeID;
	UE_LOG(LogEgnisEventCombat, Log, TEXT("Combate de evento en el nodo %s: %d accion(es) al ganar, %d al perder."),
		*NodeID.ToString(), WinActions.Num(), LoseActions.Num());
	return true;
}

void UEventCombatSubsystem::OnCombatFinished(UObject* WorldContext, bool bPlayerWon)
{
	if (!bInCombat)
	{
		return;
	}

	// Seguridad: solo vale si seguimos en el nodo del evento que lo preparo
	const UGameInstance* GI = GetGameInstance();
	const UMapProgressionSubsystem* MapSub = GI ? GI->GetSubsystem<UMapProgressionSubsystem>() : nullptr;
	if (!MapSub || MapSub->GetCurrentNode() != CombatNodeID)
	{
		UE_LOG(LogEgnisEventCombat, Warning, TEXT("El combate acabo en otro nodo distinto al del evento (%s): se descartan las acciones."), *CombatNodeID.ToString());
		ClearAll();
		return;
	}

	const TArray<FString> ToRun = bPlayerWon ? WinActions : LoseActions;
	UE_LOG(LogEgnisEventCombat, Log, TEXT("Combate de evento %s: %d accion(es)."), bPlayerWon ? TEXT("GANADO") : TEXT("PERDIDO"), ToRun.Num());

	// Se limpia antes de ejecutar para que nada se repita aunque una accion cambie la run
	ClearAll();

	for (const FString& Action : ToRun)
	{
		UEgnisEventActions::ExecuteAction(WorldContext, Action);
	}
}

void UEventCombatSubsystem::DumpToLog() const
{
	UE_LOG(LogEgnisEventCombat, Log, TEXT("[EventCombat] pedido=%d, en curso=%d, nodo=%s, nivel='%s', entorno='%s'"),
		bRequested, bInCombat, *CombatNodeID.ToString(), *LevelKey.ToString(), *Environment.ToString());
	for (const FString& Action : WinActions)  { UE_LOG(LogEgnisEventCombat, Log, TEXT("  OnWin:  %s"), *Action); }
	for (const FString& Action : LoseActions) { UE_LOG(LogEgnisEventCombat, Log, TEXT("  OnLose: %s"), *Action); }
}

static FAutoConsoleCommandWithWorldAndArgs GEgnisEventCombatDumpCmd(
	TEXT("Egnis.EventCombat.Dump"),
	TEXT("Muestra el combate de evento preparado (nivel, entorno, acciones al ganar/perder)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		if (UEventCombatSubsystem* Sub = GI ? GI->GetSubsystem<UEventCombatSubsystem>() : nullptr)
		{
			Sub->DumpToLog();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GEgnisEventCombatClearCmd(
	TEXT("Egnis.EventCombat.Clear"),
	TEXT("Descarta el combate de evento preparado"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		if (UEventCombatSubsystem* Sub = GI ? GI->GetSubsystem<UEventCombatSubsystem>() : nullptr)
		{
			Sub->ClearAll();
		}
	}));