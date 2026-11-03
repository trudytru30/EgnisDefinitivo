#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "CoreAndSystems/TileSetDataAsset.h"
#include "Characters/CharacterBase.h"
#include "Environment/ObstacleBase.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

void ABoard::GenerateBoard()
{
	ClearGeneratedBoard();

	if (!CurrentLevelData)
	{
		BuildFloor();
		UE_LOG(LogTemp, Log, TEXT("[Board]: %s no tiene CurrentLevelData: se queda con su tamaño (%d x %d) y con lo colocado a mano en el mapa"),
			*GetName(), BoardSizeX, BoardSizeY);
		return;
	}

	SyncSizeFromLevelData();
	
	TileTypes.Init(ETileType::Normal, BoardSizeX * BoardSizeY);
	for (int32 Y = 0; Y < CurrentLevelData->GridRows.Num() && Y < BoardSizeY; ++Y)
	{
		const FTileRow& Row = CurrentLevelData->GridRows[Y];
		for (int32 X = 0; X < Row.Columns.Num() && X < BoardSizeX; ++X)
		{
			TileTypes[Y * BoardSizeX + X] = Row.Columns[X];
		}
	}
	
	if (!TileSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Board]: %s no tiene Tile Set: el nivel se carga sin suelo y sin obstáculos"), *GetName());
	}
	const int32 TilesWithoutFloor = BuildFloor();
	if (TilesWithoutFloor > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Board]: %d casillas se han quedado sin cubo: en el Tile Set %s falta el cubo de su tipo y el de Normal"),
			TilesWithoutFloor, *TileSet->GetName());
	}
	
	if (TileSet)
	{
		for (int32 Y = 0; Y < BoardSizeY; ++Y)
		{
			for (int32 X = 0; X < BoardSizeX; ++X)
			{
				const FTileCoord Coord(X, Y);
				const ETileType Type = GetTileType(Coord);
				const FTileTypeDefinition* Definition = TileSet->FindDefinition(Type);
				if (Type != ETileType::Void && Definition && Definition->ActorOnTop)
				{
					SpawnActorOnTile(Coord, Definition->ActorOnTop);
				}
			}
		}
	}
	
	for (const FEnemySpawnInfo& EnemyInfo : CurrentLevelData->EnemiesToSpawn)
	{
		SpawnEnemy(EnemyInfo);
	}

	UE_LOG(LogTemp, Log, TEXT("[Board]: Nivel %s cargado: %d x %d casillas, %d cosas encima de las casillas, %d enemigos"),
		*CurrentLevelData->GetName(), BoardSizeX, BoardSizeY, SpawnedTileActors.Num(), SpawnedEnemies.Num());
}

int32 ABoard::BuildFloor()
{
	for (const TPair<ETileType, TObjectPtr<UInstancedStaticMeshComponent>>& Floor : FloorComponents)
	{
		if (Floor.Value)
		{
			Floor.Value->ClearInstances();
		}
	}

	if (!CurrentLevelData || !TileSet)
	{
		return 0;
	}
	
	const FTileTypeDefinition* NormalDefinition = TileSet->FindDefinition(ETileType::Normal);
	for (const TPair<ETileType, TObjectPtr<UInstancedStaticMeshComponent>>& Floor : FloorComponents)
	{
		UInstancedStaticMeshComponent* FloorComponent = Floor.Value;
		if (!FloorComponent)
		{
			continue;
		}

		const FTileTypeDefinition* OwnDefinition = TileSet->FindDefinition(Floor.Key);
		const FTileTypeDefinition* MeshDefinition = (OwnDefinition && OwnDefinition->FloorMesh) ? OwnDefinition : NormalDefinition;
		UMaterialInterface* Material = (OwnDefinition && OwnDefinition->FloorMaterial) ? OwnDefinition->FloorMaterial.Get()
			: (MeshDefinition ? MeshDefinition->FloorMaterial.Get() : nullptr);

		FloorComponent->SetStaticMesh(MeshDefinition ? MeshDefinition->FloorMesh.Get() : nullptr);
		FloorComponent->EmptyOverrideMaterials();
		if (Material)
		{
			FloorComponent->SetMaterial(0, Material);
		}
		FloorComponent->SetRelativeLocation(FVector(0.f, 0.f, MeshDefinition ? MeshDefinition->HeightOffset : 0.f));
	}

	int32 TilesWithoutFloor = 0;
	for (int32 Y = 0; Y < BoardSizeY; ++Y)
	{
		for (int32 X = 0; X < BoardSizeX; ++X)
		{
			const ETileType Type = GetTileType(FTileCoord(X, Y));
			if (Type == ETileType::Void)
			{
				continue;
			}

			UInstancedStaticMeshComponent* FloorComponent = FloorComponents.FindRef(Type);
			if (!FloorComponent || !FloorComponent->GetStaticMesh())
			{
				++TilesWithoutFloor;
				continue;
			}
			
			FloorComponent->AddInstance(FTransform(FVector((X + 0.5f) * TileSize, (Y + 0.5f) * TileSize, 0.f)));
		}
	}
	return TilesWithoutFloor;
}

void ABoard::SpawnActorOnTile(const FTileCoord& Tile, TSubclassOf<AActor> ActorClass)
{
	if (IsTileOccupied(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Board]: No se puede colocar %s en (%d, %d): la casilla ya está ocupada"),
			*ActorClass->GetName(), Tile.X, Tile.Y);
		return;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, TileToWorldCenter(Tile));
	AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(ActorClass, SpawnTransform, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!NewActor)
	{
		return;
	}
	
	if (AObstacleBase* Obstacle = Cast<AObstacleBase>(NewActor))
	{
		Obstacle->CurrentTile = Tile;
	}
	NewActor->FinishSpawning(SpawnTransform);

	SpawnedTileActors.Add(NewActor);
}

void ABoard::SpawnEnemy(const FEnemySpawnInfo& EnemyInfo)
{
	const FTileCoord& Tile = EnemyInfo.SpawnCoord;

	if (!EnemyInfo.EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Board]: El nivel %s tiene un enemigo sin clase en (%d, %d)"), *CurrentLevelData->GetName(), Tile.X, Tile.Y);
		return;
	}

	if (IsTileOccupied(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Board]: No se puede colocar %s en (%d, %d): está fuera del tablero u ocupada"),
			*EnemyInfo.EnemyClass->GetName(), Tile.X, Tile.Y);
		return;
	}
	
	const FTransform SpawnTransform(FRotator::ZeroRotator, TileToWorldCenter(Tile));
	AActor* NewEnemy = GetWorld()->SpawnActorDeferred<AActor>(EnemyInfo.EnemyClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!NewEnemy)
	{
		return;
	}

	if (ACharacterBase* Character = Cast<ACharacterBase>(NewEnemy))
	{
		Character->Board = this;
		Character->CurrentTile = Tile;
		Character->FinishSpawning(FTransform(FRotator::ZeroRotator, Character->GetStandLocationForTile(Tile)));
	}
	else
	{
		NewEnemy->FinishSpawning(SpawnTransform);
		RegisterOccupant(Tile, NewEnemy);
	}

	SpawnedEnemies.Add(NewEnemy);
}

void ABoard::ClearGeneratedBoard()
{
	for (AActor* TileActor : SpawnedTileActors)
	{
		if (IsValid(TileActor))
		{
			TileActor->Destroy();
		}
	}
	SpawnedTileActors.Empty();

	for (const TWeakObjectPtr<AActor>& Enemy : SpawnedEnemies)
	{
		if (AActor* EnemyActor = Enemy.Get())
		{
			EnemyActor->Destroy();
		}
	}
	SpawnedEnemies.Empty();

	TileTypes.Empty();
}

void ABoard::SyncSizeFromLevelData()
{
	if (CurrentLevelData)
	{
		BoardSizeX = FMath::Max(CurrentLevelData->Width, 1);
		BoardSizeY = FMath::Max(CurrentLevelData->Height, 1);
	}
}

ETileType ABoard::GetTileType(const FTileCoord& Tile) const
{
	if (!IsInside(Tile))
	{
		return ETileType::Void;
	}
	
	if (TileTypes.Num() == BoardSizeX * BoardSizeY)
	{
		return TileTypes[Tile.Y * BoardSizeX + Tile.X];
	}
	
	if (CurrentLevelData && CurrentLevelData->GridRows.IsValidIndex(Tile.Y) && CurrentLevelData->GridRows[Tile.Y].Columns.IsValidIndex(Tile.X))
	{
		return CurrentLevelData->GridRows[Tile.Y].Columns[Tile.X];
	}

	return ETileType::Normal;
}

void ABoard::GetBoardBounds(FVector& OutCenter, FVector2D& OutSize) const
{
	const FTransform& CornerTransform = Corner00->GetComponentTransform();
	const FVector LocalSize(BoardSizeX * TileSize, BoardSizeY * TileSize, 0.f);

	OutCenter = CornerTransform.TransformPosition(LocalSize * 0.5f);

	const FVector Scale = CornerTransform.GetScale3D();
	OutSize = FVector2D(LocalSize.X * Scale.X, LocalSize.Y * Scale.Y);
}

void ABoard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SyncSizeFromLevelData();
	BuildFloor();
}

void ABoard::PostLoad()
{
	Super::PostLoad();
	
	SyncSizeFromLevelData();
}

#if WITH_EDITOR
void ABoard::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	SyncSizeFromLevelData();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

ABoard::ABoard()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Corner00 = CreateDefaultSubobject<USceneComponent>(TEXT("Corner00"));
	Corner00->SetupAttachment(Root);
	
	const UEnum* TileTypeEnum = StaticEnum<ETileType>();
	for (int32 Index = 0; Index < TileTypeEnum->NumEnums() - 1; ++Index)
	{
		const ETileType Type = static_cast<ETileType>(TileTypeEnum->GetValueByIndex(Index));
		if (Type == ETileType::Void)
		{
			continue;
		}

		const FName ComponentName(*FString::Printf(TEXT("Floor_%s"), *TileTypeEnum->GetNameStringByIndex(Index)));
		UInstancedStaticMeshComponent* Floor = CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentName);
		Floor->SetupAttachment(Corner00);
		Floor->SetMobility(EComponentMobility::Movable);
		Floor->SetCanEverAffectNavigation(false);
		
		Floor->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		Floor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Floor->SetCollisionObjectType(ECC_WorldStatic);
		Floor->SetCollisionResponseToAllChannels(ECR_Ignore);
		Floor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

		FloorComponents.Add(Type, Floor);
	}
}

bool ABoard::WorldPointToTile(const FVector& WorldPoint, FTileCoord& OutTile) const
{
	const FVector Local = Corner00->GetComponentTransform().InverseTransformPosition(WorldPoint);
	
	const int32 X = FMath::FloorToInt(Local.X / TileSize);
	const int32 Y = FMath::FloorToInt(Local.Y / TileSize);

	const bool bValid = (X >= 0 && X < BoardSizeX && Y >= 0 && Y < BoardSizeY);

	if (!bValid)
	{
		OutTile = FTileCoord();
		return false;
	}

	OutTile.X = X;
	OutTile.Y = Y;
	return true;
}

FVector ABoard::TileToWorldCenter(const FTileCoord& Tile) const
{
	const FVector LocalCenter(
		(Tile.X + 0.5f) * TileSize,
		(Tile.Y + 0.5f) * TileSize,
		0.f
	);

	return Corner00->GetComponentTransform().TransformPosition(LocalCenter);
}

bool ABoard::IsTileOccupied(const FTileCoord& Tile) const
{
	if (!IsInside(Tile) || GetTileType(Tile) == ETileType::Void)
	{
		return true; // fuera del tablero o en un hueco (Void) se considera no caminable
	}

	if (const TWeakObjectPtr<AActor>* Found = OccupiedBy.Find(Tile))
	{
		return Found->IsValid();
	}

	return false;
}

bool ABoard::RegisterOccupant(const FTileCoord& Tile, AActor* Occupant)
{
	if (IsTileOccupied(Tile))
	{
		return false;
	}
	OccupiedBy.Add(Tile, Occupant);
	return true;
}

void ABoard::UnregisterOccupant(const FTileCoord& Tile, AActor* Occupant)
{
	if (!IsInside(Tile) || !Occupant)
	{
		return;
	}

	if (const TWeakObjectPtr<AActor>* Found = OccupiedBy.Find(Tile))
	{
		if (Found->Get() == Occupant)
		{
			OccupiedBy.Remove(Tile);
		}
	}
}

void ABoard::GetNeighbors4_Free(const FTileCoord& From, TArray<FTileCoord>& OutNeighbors) const
{
	OutNeighbors.Reset();

	const FTileCoord Candidates[4] = {
		FTileCoord{From.X + 1, From.Y},
		FTileCoord{From.X - 1, From.Y},
		FTileCoord{From.X, From.Y + 1},
		FTileCoord{From.X, From.Y - 1}
	};

	for (const FTileCoord& Candidate : Candidates)
	{
		if (IsInside(Candidate) && !IsTileOccupied(Candidate))
		{
			OutNeighbors.Add(Candidate);
		}
	}
}

AActor* ABoard::GetTileOccupant(const FTileCoord& Tile) const
{
	if (const TWeakObjectPtr<AActor>* Found = OccupiedBy.Find(Tile))
	{
		return Found->Get();
	}
	return nullptr;
}

void ABoard::SetTileOccupant(const FTileCoord& Tile, AActor* Occupant)
{
	if (Occupant)
	{
		OccupiedBy.Add(Tile, Occupant);
	}
	else
	{
		OccupiedBy.Remove(Tile);
	}
}

void ABoard::GetTilesInRange(const FTileCoord& From, int32 Range, TArray<FTileCoord>& OutTiles) const
{
	OutTiles.Reset();

	if (Range <= 0)
	{
		return;
	}

	for (int32 DX = -Range; DX <= Range; ++DX)
	{
		for (int32 DY = -Range; DY <= Range; ++DY)
		{
			const int32 Dist = FMath::Abs(DX) + FMath::Abs(DY);
			if (Dist == 0 || Dist > Range)
			{
				continue;
			}

			const FTileCoord Candidate(From.X + DX, From.Y + DY);
			if (IsInside(Candidate))
			{
				OutTiles.Add(Candidate);
			}
		}
	}
}

bool ABoard::IsInside(const FTileCoord& Tile) const
{
	return Tile.X >= 0 && Tile.X < BoardSizeX && Tile.Y >= 0 && Tile.Y < BoardSizeY;
}

void ABoard::BeginPlay()
{
	if (bGenerateOnBeginPlay)
	{
		GenerateBoard();
	}

	Super::BeginPlay();
}

void ABoard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}