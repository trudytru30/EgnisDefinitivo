#include "CoreAndSystems/Board.h"

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

