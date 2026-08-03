// Fill out your copyright notice in the Description page of Project Settings.

#include "GridMovementComponent.h"
// TODO: Descomentar cuando se migre ABoard. Solo se usaban para localizar/leer el tablero.
// #include "Engine.h"
// #include "Kismet/GameplayStatics.h"
// #include "GameFramework/Actor.h"

UGridMovementComponent::UGridMovementComponent(){}

void UGridMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// Depende de ABoard, comentado de momento. Descomentar al migrar Board.
	// EnsureBoardActor();
}

/*
 * ===== TODO: TODO EL BLOQUE SIGUIENTE DEPENDE DE ABoard / FTileCoord =====
 * Descomentar y adaptar cuando se migre Board.h al nuevo proyecto.
 * Se deja el codigo original completo como referencia.

void UGridMovementComponent::EnsureBoardActor()
{
	if (BoardActor)
	{
		return;
	}
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABoard::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		BoardActor=Cast<ABoard>(FoundActors[0]);//

		UE_LOG(LogTemp,Warning, TEXT("Tablero asignado:%s"),*GetNameSafe(BoardActor));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Tablero"));
		}

	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("No se ha encontrado ningun tablero"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red,TEXT("No se ha encontrado tablero"));

		}
	}
}

//comprueba casilla si esta dentro del tablero
bool UGridMovementComponent::IsTileValid(const FTileCoord& T)
{
	if (!BoardActor)
	{
		return false;
	}

	return (T.X >= 0 && T.X < BoardActor->BoardSizeX && T.Y >= 0 && T.Y < BoardActor->BoardSizeY);
}


//calcula la actual
bool UGridMovementComponent::getCurrentTile(FTileCoord& OutTile)
{
	if (!BoardActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("No hay BoardActor asignado"));
		return false;
	}

	return BoardActor->WorldPointToTile(GetOwner()->GetActorLocation(), OutTile);
}


// A FUTURO:
// - separar cálculo de patrón de ocupación o bloqueos
// - permitir orientación elegida por jugador (triangulo apuntando al click)
// - Extraer patrones a strategy

//devuelve tods las casillas a las que se puede mover (8 dir hasra rango)
void UGridMovementComponent::getRachableTiles(TArray<FTileCoord>& OutTiles)
{
	OutTiles.Reset();//.Empty();

	EnsureBoardActor();

	if (!BoardActor)
	{
		UE_LOG(LogTemp, Error, TEXT("No hay tablero asignado"));
		return;
	}
	FTileCoord CurrentTile;
	if (!getCurrentTile(CurrentTile))
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo obtener la casilla actual"));
		return;
	}

	switch (movePattern)
	{
	case EMovePattern::Straight_WBlock:
	case EMovePattern::Straight_CanPass:
		{
			//movimiento 8 dir
			const TArray<FTileCoord> Directions = {
				{1, 0}, {1, 1}, {0, 1}, {-1, 1},
				{-1, 0}, {-1, -1}, {0, -1}, {1, -1}
			};

			const bool bBlockByOccupant = (movePattern == EMovePattern::Straight_WBlock);

			for (const FTileCoord& Dir : Directions)
			{
				for (int32 Step = 1; Step <= moveRange; ++Step)
				{
					FTileCoord Target = { CurrentTile.X + Dir.X * Step, CurrentTile.Y + Dir.Y * Step };

					if (!IsTileValid(Target))
					{
						break;
					}

					if (BoardActor->IsTileOccupied(Target))
					{
						if (bBlockByOccupant)
						{
							break;
						}
						else
						{
							continue;
						}
					}

					OutTiles.Add(Target);
				}
			}
			break;
		}

		//patron L para Raton
	case EMovePattern::L_CanStop:
		AddLTiles(CurrentTile, OutTiles, //bCanStopAnywhere=
		true);
		break;

		//patron L para Cabeza Ardiente
	case EMovePattern::L_MustEnd:
		AddLTiles(CurrentTile, OutTiles, //bCanStopAnywhere=
		false);
		break;

		//patron triangulo
	case EMovePattern::Triangle:
		AddTriangleTiles(CurrentTile, OutTiles);
		break;

	//patorn diagonal + 1
	case EMovePattern::DiagonalPlusRing1:
		AddDiagonalPlusRing1Tiles(CurrentTile, OutTiles);
		break;

	//linea recta 4 cardinales para aliados
	case EMovePattern::StraightLine_Cardinal:
		AddStraightLineCardinalTiles(CurrentTile, OutTiles);
		break;

	default:
		break;
	}
}

bool UGridMovementComponent::canMoveToTile(const FTileCoord& Target)
{
	if (!BoardActor)
	{
		EnsureBoardActor();
	}

	if (!BoardActor)
	{
		return false;
	}
	if (!IsTileValid(Target))
	{
		return false;
	}

	if (BoardActor->IsTileOccupied(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("Casilla ocupada (%d,%d)"), Target.X, Target.Y);
		return false;
	}

	//obtiene las casillas a las que se puede mover
	TArray<FTileCoord> ReachableTiles;
	getRachableTiles(ReachableTiles);

	//comprueba si la casilla objetivo esta dentro
	for (const FTileCoord& Tile : ReachableTiles)
	{
		if (Tile.X == Target.X && Tile.Y == Target.Y)
		{
			return true;
		}
	}
	return false;
}

bool UGridMovementComponent::moveToTile(const FTileCoord& Target)
{
	EnsureBoardActor();

	if (!BoardActor)
	{
		return false;
	}

	//comprueba limites
	if (!IsTileValid(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("Casilla invalida (%d,%d)"), Target.X, Target.Y);
		return false;
	}

	//comprueba que esta dentro del patron
	if (!canMoveToTile(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("Movimiento no permitido a (%d,%d)"), Target.X, Target.Y);
		return false;
	}

	//casilla a pos en juego
	FVector Destination = BoardActor->TileToWorldCenter(Target);

	//guarda la casilla anterior para liberar
	FTileCoord OldTile;
	const bool bHasOldTile = getCurrentTile(OldTile);
	//mover actor
	GetOwner()->SetActorLocation(Destination);

	//actualiza ocupacion del tablero
	if (bHasOldTile)
	{
		BoardActor->SetTileOccupant(OldTile, nullptr);
	}
	BoardActor->SetTileOccupant(Target, GetOwner());
	UE_LOG(LogTemp, Log, TEXT("Unidad movida a (%d,%d)"), Target.X, Target.Y);

	return true;
}

void UGridMovementComponent::AddLTiles(const FTileCoord& CurrentTile, TArray<FTileCoord>& OutTiles, bool bCanStopAnywhere)
{
	//longitud total <= moveRange.
	//elige dir principal (4 cardinales) y longitud (1 a moveRange-1)
	const FTileCoord Cardinals[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

	auto GetPerpendiculars = [](const FTileCoord& D, FTileCoord& OutA, FTileCoord& OutB)
	{
		if (D.X != 0)
		{
			//si dir horizontal, perpendiculares verticales
			OutA = {0, 1}; OutB = {0, -1};
		}
		else
		{//si dir vertical, perpendiculares horizontales
			OutA = {1, 0}; OutB = {-1, 0};
		}
	};

	for (const FTileCoord& MainDir : Cardinals)
	{
		FTileCoord PerpA, PerpB;//dir perpendiculares a la dir principal
		GetPerpendiculars(MainDir, PerpA, PerpB);
		const FTileCoord Perps[2] = { PerpA, PerpB };

		for (int32 Len1 = 1; Len1 <= moveRange - 1; ++Len1)
		{
			bool bBlocked = false;
			FTileCoord Corner = CurrentTile;

			//tr1
			for (int32 i = 1; i <= Len1; ++i)
			{
				Corner = { CurrentTile.X + MainDir.X * i, CurrentTile.Y + MainDir.Y * i };

				if (!IsTileValid(Corner))
				{
					bBlocked = true;
					break;
				}

				if (BoardActor->IsTileOccupied(Corner))
				{
					bBlocked = true;
					break;
				}

				//patron ratón puede para en cualquier casilla de L
				if (bCanStopAnywhere)
				{
					OutTiles.AddUnique(Corner);
				}
			}

			if (bBlocked)
			{
				continue;
			}

			//tr2
			for (const FTileCoord& PerpDir : Perps)
			{
				//la esquina como punto de partida para la parte perpendicular
				FTileCoord End = Corner;

				for (int32 Len2 = 1; Len2 <= (moveRange - Len1); ++Len2)
				{
					End = { Corner.X + PerpDir.X * Len2, Corner.Y + PerpDir.Y * Len2 };

					if (!IsTileValid(End))
					{
						break;
					}

					if (BoardActor->IsTileOccupied(End))
					{
						break;
					}

					if (bCanStopAnywhere)
					{
						//para ratón puede parar en cualquier casilla
						OutTiles.AddUnique(End);
					}
					else
					{
						//para cabezas ardientes solo final tr 2 no tr1
						OutTiles.AddUnique(End);
					}
				}
			}
		}
	}
}
void UGridMovementComponent::AddTriangleTiles(const FTileCoord& CurrentTile, TArray<FTileCoord>& OutTiles)
{
	EnsureBoardActor();
	if (!BoardActor) return;

	//en las 4 dir
	const FTileCoord ForwardDirs[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

	for (const FTileCoord& Fwd : ForwardDirs)
	{
		//cada distancia
		for (int32 Dist = 1; Dist <= moveRange; ++Dist)
		{
			//ancho triangulo ,dist hacia adelante
			for (int32 Side = -Dist; Side <= Dist; ++Side)
			{
				FTileCoord Target = CurrentTile;

				if (Fwd.X != 0)
				{
					//triangulo apuntando en X: avanza en X y abre en Y
					Target.X += Fwd.X * Dist;
					Target.Y += Side;
				}
				else
				{
					//traingulo apunta en Y: avanza en Y y abre en X
					Target.Y += Fwd.Y * Dist;
					Target.X += Side;
				}

				if (!IsTileValid(Target))
					continue;

				if (BoardActor->IsTileOccupied(Target))
					continue;

				OutTiles.AddUnique(Target);
			}
		}
	}
}
void UGridMovementComponent::AddStraightLineCardinalTiles(const FTileCoord& CurrentTile, TArray<FTileCoord>& OutTiles)
{
	EnsureBoardActor();
	if (!BoardActor) return;

	// 4 direcciones cardinales: arriba, abajo, izquierda, derecha
	const FTileCoord Cardinals[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

	for (const FTileCoord& Dir : Cardinals)
	{
		for (int32 Step = 1; Step <= moveRange; ++Step)
		{
			FTileCoord Target = { CurrentTile.X + Dir.X * Step, CurrentTile.Y + Dir.Y * Step };

			if (!IsTileValid(Target))
			{
				break;
			}

			if (BoardActor->IsTileOccupied(Target))
			{
				break;
			}

			OutTiles.Add(Target);
		}
	}
}

void UGridMovementComponent::AddDiagonalPlusRing1Tiles(const FTileCoord& CurrentTile, TArray<FTileCoord>& OutTiles)
{
	EnsureBoardActor();
	if (!BoardActor) return;

	//1.diagonales en rango de 4 dir
	const FTileCoord Diagonals[4] = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };

	for (const FTileCoord& Dir : Diagonals)
	{
		//cada distancia en diagonal
		for (int32 Step = 1; Step <= moveRange; ++Step)
		{
			FTileCoord Target(CurrentTile.X + Dir.X * Step, CurrentTile.Y + Dir.Y * Step);

			if (!IsTileValid(Target))
				break;

			if (BoardActor->IsTileOccupied(Target))
			{
				break;
			}

			OutTiles.AddUnique(Target);
		}
	}

	//2.anillo en 8 dir
	const FTileCoord Ring8[8] =
	{
		{1,0}, {1,1}, {0,1}, {-1,1},
		{-1,0}, {-1,-1}, {0,-1}, {1,-1}
	};

	for (const FTileCoord& Ofs : Ring8)
	{
		//casilla objetivo
		FTileCoord Target(CurrentTile.X + Ofs.X, CurrentTile.Y + Ofs.Y);

		if (!IsTileValid(Target))
		{
			continue;
		}

		if (BoardActor->IsTileOccupied(Target))
		{
			continue;
		}

		OutTiles.AddUnique(Target);
	}
}

 * ===== FIN DEL BLOQUE COMENTADO =====
 */