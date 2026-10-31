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
	bWarnedNoDeployZones = false;
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
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: %s ya está en el tablero, se ha llegado al límite (%d) o no queda hueco en ninguna zona"), *SelectedHero->GetName(), MaxDeployedHeroes);
		return false;
	}

	if (!IsTileInFreeDeployZone(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: La casilla (%d, %d) no es de ninguna zona de despliegue o su zona está llena"), Tile.X, Tile.Y);
		return false;
	}

	if (!LevelHasDeployZones() && !bWarnedNoDeployZones)
	{
		bWarnedNoDeployZones = true;
		UE_LOG(LogTemp, Warning, TEXT("[DeployManager]: El tablero no tiene CurrentLevelData o su nivel no tiene zonas: se puede desplegar en todo el tablero"));
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

	//con zonas, además tiene que quedar alguna casilla libre en una zona con hueco
	TArray<FTileCoord> DeployableTiles;
	return !LevelHasDeployZones() || (GetDeployableTiles(DeployableTiles) && DeployableTiles.Num() > 0);
}

bool UDeployManager::IsTileInFreeDeployZone(const FTileCoord& Tile) const
{
	if (!LevelHasDeployZones())
	{
		return true;
	}

	const ULevelDataAsset* LevelData = GetLevelData();
	const int32 ZoneIndex = LevelData->FindDeployZoneIndex(Tile);
	return ZoneIndex != INDEX_NONE && GetNumHeroesInZone(ZoneIndex) < LevelData->DeployZones[ZoneIndex].MaxHeroes;
}

bool UDeployManager::GetDeployableTiles(TArray<FTileCoord>& OutTiles) const
{
	OutTiles.Reset();
	if (!LevelHasDeployZones())
	{
		return false;
	}

	const ULevelDataAsset* LevelData = GetLevelData();
	const ABoard* Board = GetBoard();
	for (int32 ZoneIndex = 0; ZoneIndex < LevelData->DeployZones.Num(); ++ZoneIndex)
	{
		const FDeployZone& Zone = LevelData->DeployZones[ZoneIndex];
		if (GetNumHeroesInZone(ZoneIndex) >= Zone.MaxHeroes)
		{
			continue;
		}

		for (const FTileCoord& Tile : Zone.Tiles)
		{
			if (!Board->IsTileOccupied(Tile))
			{
				OutTiles.Add(Tile);
			}
		}
	}
	return true;
}

int32 UDeployManager::GetNumHeroesInZone(int32 ZoneIndex) const
{
	const ULevelDataAsset* LevelData = GetLevelData();
	if (!LevelData || !LevelData->DeployZones.IsValidIndex(ZoneIndex))
	{
		return 0;
	}

	//No se guarda un contador por zona: se calcula con las casillas de los héroes desplegados,
	//así al quitar uno con clic derecho su hueco se libera solo
	int32 Count = 0;
	for (const TPair<TWeakObjectPtr<AAlly>, FName>& Deployed : DeployedHeroes)
	{
		const AAlly* Ally = Deployed.Key.Get();
		if (Ally && LevelData->FindDeployZoneIndex(Ally->CurrentTile) == ZoneIndex)
		{
			++Count;
		}
	}
	return Count;
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

bool UDeployManager::LevelHasDeployZones() const
{
	const ULevelDataAsset* LevelData = GetLevelData();
	return LevelData && LevelData->DeployZones.Num() > 0;
}
