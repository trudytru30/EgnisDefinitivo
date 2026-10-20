#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "Characters/CharacterBase.h"

void ABoard::GenerateBoard()
{
	ClearGeneratedBoard();

	if (!CurrentLevelData)
	{
		UE_LOG(LogTemp, Log, TEXT("[Board]: %s no tiene CurrentLevelData: se queda con su tamaño (%d x %d) y con lo colocado a mano en el mapa"),
			*GetName(), BoardSizeX, BoardSizeY);
		return;
	}

	SyncSizeFromLevelData();

	// 1. TIPOS DE CASILLA: la matriz del asset es GridRows[Y].Columns[X]
	TileTypes.Init(ETileType::Normal, BoardSizeX * BoardSizeY);
	for (int32 Y = 0; Y < CurrentLevelData->GridRows.Num() && Y < BoardSizeY; ++Y)
	{
		const FTileRow& Row = CurrentLevelData->GridRows[Y];
		for (int32 X = 0; X < Row.Columns.Num() && X < BoardSizeX; ++X)
		{
			TileTypes[Y * BoardSizeX + X] = Row.Columns[X];
		}
	}

	// 2. VISUAL DE CADA CASILLA, si su tipo tiene uno en TileVisuals (Void nunca: es un hueco)
	for (int32 Y = 0; Y < BoardSizeY; ++Y)
	{
		for (int32 X = 0; X < BoardSizeX; ++X)
		{
			const FTileCoord Coord(X, Y);
			const ETileType Type = GetTileType(Coord);
			if (Type == ETileType::Void)
			{
				continue;
			}

			const TSubclassOf<AActor>* TileClass = TileVisuals.Find(Type);
			if (!TileClass || !*TileClass)
			{
				continue;
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			AActor* NewTile = GetWorld()->SpawnActor<AActor>(*TileClass, TileToWorldCenter(Coord), FRotator::ZeroRotator, SpawnParams);
			if (NewTile)
			{
				NewTile->AttachToComponent(Corner00, FAttachmentTransformRules::KeepWorldTransform);
				SpawnedTiles.Add(NewTile);
			}
		}
	}

	// 3. ENEMIGOS
	for (const FEnemySpawnInfo& EnemyInfo : CurrentLevelData->EnemiesToSpawn)
	{
		SpawnEnemy(EnemyInfo);
	}

	UE_LOG(LogTemp, Log, TEXT("[Board]: Nivel %s cargado: %d x %d casillas, %d visuales de casilla, %d enemigos"),
		*CurrentLevelData->GetName(), BoardSizeX, BoardSizeY, SpawnedTiles.Num(), SpawnedEnemies.Num());
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

	//Spawn diferido, igual que el despliegue de héroes (DeployManager): el tablero y la casilla se asignan
	//ANTES del BeginPlay, que es donde el personaje se registra como ocupante
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
		//ya sabe su tablero y su casilla: termina de spawnear directamente de pie sobre ella
		Character->FinishSpawning(FTransform(FRotator::ZeroRotator, Character->GetStandLocationForTile(Tile)));
	}
	else
	{
		//no es un personaje: no se registra solo, lo apunta el tablero
		NewEnemy->FinishSpawning(SpawnTransform);
		RegisterOccupant(Tile, NewEnemy);
	}

	SpawnedEnemies.Add(NewEnemy);
}

void ABoard::ClearGeneratedBoard()
{
	for (AActor* Tile : SpawnedTiles)
	{
		if (Tile) Tile->Destroy();
	}
	SpawnedTiles.Empty();

	//No se vacía OccupiedBy entero: borraría a los personajes y obstáculos colocados a mano en el mapa.
	//Cada enemigo deja libre su casilla al destruirse: los personajes se quitan en su EndPlay, y para el resto
	//su entrada en OccupiedBy (un TWeakObjectPtr) deja de ser válida
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

	//En partida: la copia que hizo GenerateBoard
	if (TileTypes.Num() == BoardSizeX * BoardSizeY)
	{
		return TileTypes[Tile.Y * BoardSizeX + Tile.X];
	}

	//En el editor (sin generar todavía): directamente del asset
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

void ABoard::PostLoad()
{
	Super::PostLoad();

	//Al abrir el mapa el tamaño ya tiene que ser el del nivel: los personajes colocados a mano
	//pueden registrarse antes del BeginPlay del tablero
	SyncSizeFromLevelData();
}

#if WITH_EDITOR
void ABoard::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//Antes del Super: el Super vuelve a ejecutar el construction script del BP (que dibuja la rejilla),
	//y tiene que ver ya el tamaño del nivel. OnConstruction no sirve: el motor lo llama DESPUÉS del construction script
	SyncSizeFromLevelData();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

ABoard::ABoard()
{
	// Este actor no necesita tick por ahora.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Corner00 = CreateDefaultSubobject<USceneComponent>(TEXT("Corner00"));
	Corner00->SetupAttachment(Root);
}

bool ABoard::WorldPointToTile(const FVector& WorldPoint, FTileCoord& OutTile) const
{
	const FVector Local = Corner00->GetComponentTransform().InverseTransformPosition(WorldPoint);

	// Redondea hacia abajo a la casilla que contiene el punto
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
	if (!IsInside(Tile))
	{
		return true; // fuera del tablero se considera no caminable
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
	// Unificado con OccupiedBy (ver nota en el .h). Antes escribia en un TMap
	// distinto al que usan IsTileOccupied/RegisterOccupant/UnregisterOccupant.
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
	//Antes del Super, que es quien lanza el Event BeginPlay del BP: así el BP ya ve el nivel cargado.
	//Los enemigos que se crean aquí hacen su BeginPlay al momento (el motor lo permite dentro de otro BeginPlay)
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