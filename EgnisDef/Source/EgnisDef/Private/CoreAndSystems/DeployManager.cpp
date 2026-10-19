#include "CoreAndSystems/DeployManager.h"
#include "Characters/Ally.h"
#include "Characters/HeroDataAsset.h"
#include "CoreAndSystems/RunProgressionSubsystem.h"

void UDeployManager::Initialize(TSubclassOf<AAlly> InBaseAllyClass, int32 InMaxDeployedHeroes)
{
	BaseAllyClass = InBaseAllyClass;
	MaxDeployedHeroes = FMath::Max(1, InMaxDeployedHeroes);
	SelectedHero = nullptr;
	DeployedHeroes.Empty();//lista nueva en cada despliegue
}

bool UDeployManager::TryDeployHeroAtTile(ABoard* Board, const FTileCoord& Tile)
{
	if (!Board || !BaseAllyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: Falta el tablero o BaseAllyClass"));
		return false;
	}

	if (!SelectedHero)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: No hay ningún héroe seleccionado"));
		return false;
	}

	//Las reglas se comprueban aquí aunque la UI ya desactive los botones: así nadie se las puede saltar
	if (!CanDeployHero(SelectedHero))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: %s ya está en el tablero o se ha llegado al límite (%d)"), *SelectedHero->GetName(), MaxDeployedHeroes);
		return false;
	}

	if (Board->IsTileOccupied(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: La casilla (%d, %d) está ocupada o fuera del tablero"), Tile.X, Tile.Y);
		return false;
	}

	//Spawn diferido: el tablero y la casilla se asignan ANTES del BeginPlay,
	//que es donde el personaje se registra como ocupante y se centra en su casilla
	const FTransform SpawnTransform(FRotator::ZeroRotator, Board->TileToWorldCenter(Tile));
	AAlly* SpawnedAlly = GetWorld()->SpawnActorDeferred<AAlly>(BaseAllyClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!SpawnedAlly)
	{
		return false;
	}

	SpawnedAlly->Board = Board;
	SpawnedAlly->CurrentTile = Tile;
	//ya sabe su tablero y su casilla: termina de spawnear directamente de pie sobre ella
	SpawnedAlly->FinishSpawning(FTransform(FRotator::ZeroRotator, SpawnedAlly->GetStandLocationForTile(Tile)));

	SpawnedAlly->InitializeFromData(SelectedHero);

	//apunta qué héroe es este personaje del tablero (para las heridas al acabar el combate)
	DeployedHeroes.Emplace(SpawnedAlly, URunProgressionSubsystem::GetHeroId(SelectedHero));
	UE_LOG(LogTemp, Log, TEXT("[DeployManager]: %s desplegado como %s en (%d, %d)"), *SpawnedAlly->GetName(), *SelectedHero->GetName(), Tile.X, Tile.Y);

	SelectedHero = nullptr;
	OnDeploymentChanged.Broadcast();
	return true;
}

bool UDeployManager::RemoveDeployedHero(AAlly* Ally)
{
	if (!Ally)
	{
		return false;
	}

	const int32 Index = DeployedHeroes.IndexOfByPredicate([Ally](const TPair<TWeakObjectPtr<AAlly>, FName>& Deployed)
	{
		return Deployed.Key.Get() == Ally;
	});

	if (Index == INDEX_NONE)
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[DeployManager]: %s retirado del tablero"), *DeployedHeroes[Index].Value.ToString());

	DeployedHeroes.RemoveAt(Index);
	Ally->Destroy();//su EndPlay lo quita del tablero

	OnDeploymentChanged.Broadcast();
	return true;
}

bool UDeployManager::IsHeroDeployed(const UHeroDataAsset* Hero) const
{
	const FName HeroId = URunProgressionSubsystem::GetHeroId(Hero);
	if (HeroId.IsNone())
	{
		return false;
	}

	return DeployedHeroes.ContainsByPredicate([HeroId](const TPair<TWeakObjectPtr<AAlly>, FName>& Deployed)
	{
		return Deployed.Value == HeroId;
	});
}

bool UDeployManager::CanDeployHero(const UHeroDataAsset* Hero) const
{
	return Hero && !IsHeroDeployed(Hero) && DeployedHeroes.Num() < MaxDeployedHeroes;
}
