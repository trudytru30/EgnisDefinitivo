#include "CoreAndSystems/DeployManager.h"
#include "Characters/Ally.h"
#include "Characters/HeroDataAsset.h"
#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "Kismet/GameplayStatics.h"

void UDeployManager::Initialize(TSubclassOf<AAlly> InBaseAllyClass, int32 InMaxDeployedHeroes)
{
	BaseAllyClass = InBaseAllyClass;
	MaxDeployedHeroes = FMath::Max(1, InMaxDeployedHeroes);
	SelectedHero = nullptr;
	DeployedHeroes.Empty();//lista nueva en cada despliegue
	bWarnedNoLevelData = false;
}

bool UDeployManager::TryDeployHeroAtTile(ABoard* Board, const FTileCoord& Tile)
{
	if (!Board || !BaseAllyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: Falta el tablero o BaseAllyClass"));
		return false;
	}
	CachedBoard = Board;

	if (!SelectedHero)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: No hay ningún héroe seleccionado"));
		return false;
	}

	//Las reglas se comprueban aquí aunque la UI ya desactive los botones: así nadie se las puede saltar
	if (!CanDeployHero(SelectedHero))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: %s ya está en el tablero, se ha llegado al límite (%d) o no queda ninguna casilla donde colocarlo"), *SelectedHero->GetName(), MaxDeployedHeroes);
		return false;
	}

	if (!GetLevelData() && !bWarnedNoLevelData)
	{
		bWarnedNoLevelData = true;
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: El tablero no tiene CurrentLevelData: se puede desplegar en cualquier casilla libre"));
	}

	if (!CanDeployOnTile(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: No se puede colocar en (%d, %d): está ocupada, fuera del tablero o a %d casillas o menos de un enemigo"),
			Tile.X, Tile.Y, GetLevelData() ? GetLevelData()->DeployMinDistanceToEnemies : 0);
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
	SpawnedAlly->HeroAsset = SelectedHero;
	//ya sabe su tablero y su casilla: termina de spawnear directamente de pie sobre ella
	SpawnedAlly->FinishSpawning(FTransform(FRotator::ZeroRotator, SpawnedAlly->GetStandLocationForTile(Tile)));

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
	if (!Hero || IsHeroDeployed(Hero) || DeployedHeroes.Num() >= MaxDeployedHeroes)
	{
		return false;
	}

	//además tiene que quedar alguna casilla libre donde colocarlo
	TArray<FTileCoord> DeployableTiles;
	return GetDeployableTiles(DeployableTiles) && DeployableTiles.Num() > 0;
}

bool UDeployManager::CanDeployOnTile(const FTileCoord& Tile) const
{
	const ABoard* Board = GetBoard();
	if (!Board || Board->IsTileOccupied(Tile))
	{
		return false;
	}

	const ULevelDataAsset* LevelData = GetLevelData();
	return !LevelData || LevelData->CanDeployOnTile(Tile);
}

bool UDeployManager::GetDeployableTiles(TArray<FTileCoord>& OutTiles) const
{
	OutTiles.Reset();
	const ABoard* Board = GetBoard();
	if (!Board)
	{
		return false;
	}

	for (int32 Y = 0; Y < Board->BoardSizeY; ++Y)
	{
		for (int32 X = 0; X < Board->BoardSizeX; ++X)
		{
			const FTileCoord Tile(X, Y);
			if (CanDeployOnTile(Tile))
			{
				OutTiles.Add(Tile);
			}
		}
	}
	return true;
}

ABoard* UDeployManager::GetBoard() const
{
	if (!CachedBoard.IsValid())
	{
		CachedBoard = Cast<ABoard>(UGameplayStatics::GetActorOfClass(GetWorld(), ABoard::StaticClass()));
	}
	return CachedBoard.Get();
}

const ULevelDataAsset* UDeployManager::GetLevelData() const
{
	const ABoard* Board = GetBoard();
	return Board ? Board->CurrentLevelData : nullptr;
}
