#include "Map/EventPoolSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Map/EventPoolDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisEventPool, Log, All);

namespace
{
	FString MakeNodeKey(EMapSection Section, FName NodeID)
	{
		return FString::Printf(TEXT("%d/%s"), static_cast<int32>(Section), *NodeID.ToString());
	}
}

void UEventPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (URunProgressionSubsystem* Run = Collection.InitializeDependency<URunProgressionSubsystem>())
	{
		Run->OnRunChanged.AddDynamic(this, &UEventPoolSubsystem::HandleRunChanged);
	}
}

void UEventPoolSubsystem::HandleRunChanged()
{
	// Al cerrarse la run, la siguiente empieza con todos los eventos disponibles otra vez
	const UGameInstance* GI = GetGameInstance();
	const URunProgressionSubsystem* Run = GI ? GI->GetSubsystem<URunProgressionSubsystem>() : nullptr;
	if (Run && !Run->IsRunActive() && (UseCount.Num() > 0 || ResolvedByNode.Num() > 0))
	{
		ResetAll();
	}
}

void UEventPoolSubsystem::ResetAll()
{
	UseCount.Reset();
	ResolvedByNode.Reset();
	UE_LOG(LogEgnisEventPool, Log, TEXT("[EventPool] reiniciada."));
}

void UEventPoolSubsystem::LoadPools()
{
	bPoolsLoaded = true;
	Pools.Reset();

	FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	Module.Get().GetAssetsByClass(UEventPoolDataAsset::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Data : Assets)
	{
		if (UEventPoolDataAsset* Pool = Cast<UEventPoolDataAsset>(Data.GetAsset()))
		{
			Pools.Add(Pool);
		}
	}
	UE_LOG(LogEgnisEventPool, Log, TEXT("[EventPool] %d pools encontradas."), Pools.Num());
}

const UEventPoolDataAsset* UEventPoolSubsystem::FindPool(EMapSection Section) const
{
	const UEventPoolDataAsset* Found = nullptr;
	for (const UEventPoolDataAsset* Pool : Pools)
	{
		if (Pool && Pool->Section == Section)
		{
			if (Found)
			{
				UE_LOG(LogEgnisEventPool, Warning, TEXT("[EventPool] hay mas de una pool para la zona %d (%s y %s). Se usa la primera."),
					static_cast<int32>(Section), *Found->GetName(), *Pool->GetName());
				break;
			}
			Found = Pool;
		}
	}
	return Found;
}

bool UEventPoolSubsystem::ResolveEventForNode(FName NodeID, EMapSection Section, FName& OutEventID)
{
	const FString Key = MakeNodeKey(Section, NodeID);
	if (const FName* Already = ResolvedByNode.Find(Key))
	{
		OutEventID = *Already;
		return true;
	}

	if (!bPoolsLoaded)
	{
		LoadPools();
	}

	const UEventPoolDataAsset* Pool = FindPool(Section);
	if (!Pool)
	{
		UE_LOG(LogEgnisEventPool, Warning, TEXT("[EventPool] no hay pool para la zona %d (nodo %s)."), static_cast<int32>(Section), *NodeID.ToString());
		return false;
	}

	// Peso efectivo = peso base x multiplicador^veces que ya ha salido en esta run
	TArray<const FEventPoolEntry*> Candidates;
	TArray<float> EffectiveWeights;
	float TotalWeight = 0.f;
	auto CollectCandidates = [&]()
	{
		Candidates.Reset();
		EffectiveWeights.Reset();
		TotalWeight = 0.f;
		for (const FEventPoolEntry& Entry : Pool->Events)
		{
			if (Entry.EventID.IsNone() || Entry.Weight <= 0.f)
			{
				continue;
			}
			const int32 Uses = UseCount.FindRef(Entry.EventID);
			const float Effective = Entry.Weight * FMath::Pow(Pool->RepeatWeightMultiplier, static_cast<float>(Uses));
			if (Effective <= 0.f)
			{
				continue;
			}
			Candidates.Add(&Entry);
			EffectiveWeights.Add(Effective);
			TotalWeight += Effective;
		}
	};

	CollectCandidates();
	if (Candidates.Num() == 0)
	{
		// Con multiplicador 0 y todos ya usados: vuelven a estar disponibles
		for (const FEventPoolEntry& Entry : Pool->Events)
		{
			UseCount.Remove(Entry.EventID);
		}
		UE_LOG(LogEgnisEventPool, Warning, TEXT("[EventPool] pool '%s' agotada: se reinician las veces usadas."), *Pool->GetName());
		CollectCandidates();
	}
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogEgnisEventPool, Warning, TEXT("[EventPool] la pool '%s' no tiene eventos validos."), *Pool->GetName());
		return false;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	int32 ChosenIndex = Candidates.Num() - 1;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		Roll -= EffectiveWeights[Index];
		if (Roll <= 0.f)
		{
			ChosenIndex = Index;
			break;
		}
	}

	const FEventPoolEntry* Chosen = Candidates[ChosenIndex];
	UE_LOG(LogEgnisEventPool, Log, TEXT("[EventPool] nodo %s (zona %d) -> %s  (probabilidad %.0f%%, ya salio %d veces)"),
		*NodeID.ToString(), static_cast<int32>(Section), *Chosen->EventID.ToString(),
		100.f * EffectiveWeights[ChosenIndex] / TotalWeight, UseCount.FindRef(Chosen->EventID));

	UseCount.FindOrAdd(Chosen->EventID) += 1;
	ResolvedByNode.Add(Key, Chosen->EventID);
	OutEventID = Chosen->EventID;
	return true;
}

void UEventPoolSubsystem::DumpToLog() const
{
	UE_LOG(LogEgnisEventPool, Log, TEXT("[EventPool] %d pools cargadas, %d eventos distintos usados, %d nodos asignados."),
		Pools.Num(), UseCount.Num(), ResolvedByNode.Num());
	for (const UEventPoolDataAsset* Pool : Pools)
	{
		if (!Pool) continue;
		UE_LOG(LogEgnisEventPool, Log, TEXT("  Pool '%s' (zona %d), multiplicador de repeticion %.2f:"),
			*Pool->GetName(), static_cast<int32>(Pool->Section), Pool->RepeatWeightMultiplier);
		for (const FEventPoolEntry& Entry : Pool->Events)
		{
			const int32 Uses = UseCount.FindRef(Entry.EventID);
			UE_LOG(LogEgnisEventPool, Log, TEXT("    %s  peso base %.1f, peso actual %.2f, salio %d veces"), *Entry.EventID.ToString(), Entry.Weight,
				Entry.Weight * FMath::Pow(Pool->RepeatWeightMultiplier, static_cast<float>(Uses)), Uses);
		}
	}
	for (const TPair<FString, FName>& Pair : ResolvedByNode)
	{
		UE_LOG(LogEgnisEventPool, Log, TEXT("  nodo %s -> %s"), *Pair.Key, *Pair.Value.ToString());
	}
}

namespace
{
	UEventPoolSubsystem* GetPoolSubsystem(UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UEventPoolSubsystem>() : nullptr;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GEgnisEventsDumpCmd(
	TEXT("Egnis.Events.Dump"),
	TEXT("Muestra las pools de eventos y cuales se han usado"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		if (UEventPoolSubsystem* Sub = GetPoolSubsystem(World))
		{
			Sub->DumpToLog();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GEgnisEventsResetCmd(
	TEXT("Egnis.Events.Reset"),
	TEXT("Reinicia los eventos usados de la pool"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		if (UEventPoolSubsystem* Sub = GetPoolSubsystem(World))
		{
			Sub->ResetAll();
		}
	}));

// Para probar sin el mapa: Egnis.Events.Draw 0   (0 = zona 1, 1 = zona 2, 2 = zona 3)
static FAutoConsoleCommandWithWorldAndArgs GEgnisEventsDrawCmd(
	TEXT("Egnis.Events.Draw"),
	TEXT("Saca un evento de la pool de una zona (0, 1 o 2). Cada llamada usa un nodo de prueba nuevo."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		static int32 DebugCounter = 0;
		UEventPoolSubsystem* Sub = GetPoolSubsystem(World);
		if (!Sub)
		{
			return;
		}
		const int32 Zone = Args.IsValidIndex(0) ? FMath::Clamp(FCString::Atoi(*Args[0]), 0, 2) : 0;
		FName EventID;
		const FName DebugNode(*FString::Printf(TEXT("Debug_%d"), ++DebugCounter));
		if (Sub->ResolveEventForNode(DebugNode, static_cast<EMapSection>(Zone), EventID))
		{
			UE_LOG(LogEgnisEventPool, Log, TEXT("Egnis.Events.Draw: %s"), *EventID.ToString());
		}
	}));