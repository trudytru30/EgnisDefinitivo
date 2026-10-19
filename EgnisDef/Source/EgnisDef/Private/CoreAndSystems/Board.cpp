#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/LevelDataAsset.h"

void ABoard::GenerateBoard()
{
    if (!CurrentLevelData)
    {
        UE_LOG(LogTemp, Warning, TEXT("Generación cancelada: No hay LevelDataAsset asignado en %s"), *GetName());
        return;
    }

    // Limpiar el tablero anterior si ya existía
    for (AActor* Tile : SpawnedTiles)
    {
        if (Tile) Tile->Destroy();
    }
    SpawnedTiles.Empty();
    OccupiedBy.Empty();

    // 1. GENERAR CASILLAS LEYENDO LA MATRIZ (GridRows[Y].Columns[X]) DEL DATA ASSET
    for (int32 Y = 0; Y < CurrentLevelData->GridRows.Num() && Y < BoardSizeY; ++Y)
    {
        const FTileRow& Row = CurrentLevelData->GridRows[Y];

        for (int32 X = 0; X < Row.Columns.Num() && X < BoardSizeX; ++X)
        {
            FTileCoord CurrentCoord(X, Y);
            ETileType TypeToSpawn = Row.Columns[X];

            // Si el tipo es Void, podemos saltárnoslo para que no spawnee nada en ese hueco
            if (TypeToSpawn == ETileType::Void)
            {
                continue;
            }

            // Buscamos el Blueprint en el mapa global de visuales del propio Tablero (ABoard)
            if (TSubclassOf<AActor>* TileClass = TileVisuals.Find(TypeToSpawn))
            {
                if (*TileClass)
                {
                    FVector SpawnLocation = TileToWorldCenter(CurrentCoord);
                    FActorSpawnParameters SpawnParams;
                    SpawnParams.Owner = this;

                    // Instanciamos la casilla
                    AActor* NewTile = GetWorld()->SpawnActor<AActor>(*TileClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
                    if (NewTile)
                    {
                        NewTile->AttachToComponent(Corner00, FAttachmentTransformRules::KeepWorldTransform);
                        SpawnedTiles.Add(NewTile);
                    }
                }
            }
        }
    }

    // 2. GENERAR ENEMIGOS
    for (const FEnemySpawnInfo& EnemyInfo : CurrentLevelData->EnemiesToSpawn)
    {
        if (EnemyInfo.EnemyClass && IsInside(EnemyInfo.SpawnCoord))
        {
            FVector SpawnLocation = TileToWorldCenter(EnemyInfo.SpawnCoord);
            FActorSpawnParameters SpawnParams;

            AActor* NewEnemy = GetWorld()->SpawnActor<AActor>(EnemyInfo.EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
            if (NewEnemy)
            {
                RegisterOccupant(EnemyInfo.SpawnCoord, NewEnemy);
            }
        }
    }
}

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
	Super::BeginPlay();
}

void ABoard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}