#include "VFX/Card_IvyVFXEffect.h"

#include "Characters/CharacterBase.h"
#include "CoreAndSystems/Board.h"
#include "Environment/ObstacleBase.h"
#include "VFX/IvyVFXActor.h"
#include "VFX/IvyVFXSettings.h"

UCard_IvyVFXEffect::UCard_IvyVFXEffect()
{
	// Como StatusEffect: por defecto sobre un enemigo; se cambia a Ally, Self o Tile segun la carta.
	Target = ECardTarget::Enemy;
}

AIvyVFXActor* UCard_IvyVFXEffect::PlayIvyThroughPieces(ACharacterBase* WorldContext, UIvyVFXSettings* InSettings, const TArray<AActor*>& Pieces)
{
	if (!IsValid(WorldContext) || !InSettings)
	{
		UE_LOG(LogTemp, Warning, TEXT("IvyVFX: falta el personaje de contexto o el DataAsset de Settings, no se lanza la hiedra."));
		return nullptr;
	}
	return AIvyVFXActor::SpawnIvyThroughActors(WorldContext, InSettings, Pieces);
}

void UCard_IvyVFXEffect::Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy)
{
	Super::Execute_Enemy(Self, Enemy);
	if (IsValid(Enemy))
	{
		Play(Self, Enemy, nullptr);
	}
}

void UCard_IvyVFXEffect::Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally)
{
	Super::Execute_Ally(Self, Ally);
	if (IsValid(Ally))
	{
		Play(Self, Ally, nullptr);
	}
}

void UCard_IvyVFXEffect::Execute_Obstacle(ACharacterBase* Self, AObstacleBase* Obstacle)
{
	Super::Execute_Obstacle(Self, Obstacle);
	if (IsValid(Obstacle))
	{
		Play(Self, Obstacle, nullptr);
	}
}

void UCard_IvyVFXEffect::Execute_Self(ACharacterBase* Source)
{
	Super::Execute_Self(Source);
	Play(Source, Source, nullptr);
}

void UCard_IvyVFXEffect::Execute_Tile(ACharacterBase* Self, FVector Location)
{
	Super::Execute_Tile(Self, Location);
	Play(Self, nullptr, &Location);
}

void UCard_IvyVFXEffect::Play(ACharacterBase* Self, AActor* TargetActor, const FVector* TileLocation)
{
	// IsValid y no solo != nullptr: otro efecto de la misma carta pudo matar ya a alguien.
	if (!IsValid(Self))
	{
		UE_LOG(LogTemp, Warning, TEXT("IvyVFX: quien juega la carta no es valido, no se lanza la hiedra."));
		return;
	}
	if (!Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("IvyVFX: el efecto de la carta no tiene DataAsset de Settings asignado."));
		return;
	}

	TArray<FIvyNode> Nodes;
	const FIvyNode CasterNode = AIvyVFXActor::MakeIvyNodeFromActor(Self, Settings);
	if (bStartFromCaster)
	{
		Nodes.Add(CasterNode);
	}

	FVector TargetLocation = Self->GetActorLocation();
	if (IsValid(TargetActor))
	{
		TargetLocation = TargetActor->GetActorLocation();
		// Si el objetivo es quien juega la carta ya esta en la lista (Execute_Self): un solo nodo, se enreda en si mismo.
		if (!(bStartFromCaster && TargetActor == Self))
		{
			Nodes.Add(AIvyVFXActor::MakeIvyNodeFromActor(TargetActor, Settings));
		}
	}
	else if (TileLocation)
	{
		TargetLocation = *TileLocation;
		// La casilla se toma a ras del suelo de quien juega la carta (los Location de los clics pueden traer otra altura)
		// y no tiene nada por lo que trepar: la hiedra la rodea a ras de suelo.
		FIvyNode& TileNode = Nodes.AddDefaulted_GetRef();
		TileNode.Location = FVector(TileLocation->X, TileLocation->Y, CasterNode.Location.Z);
		TileNode.Height = 0.f;
	}

	if (Nodes.Num() == 0)
	{
		return;
	}

	// Propagacion visual por piezas cercanas al objetivo
	if (PropagationRadius > 0)
	{
		TArray<AActor*> Chain;
		CollectPropagation(Self, TargetActor, TargetLocation, Chain);
		for (AActor* Piece : Chain)
		{
			Nodes.Add(AIvyVFXActor::MakeIvyNodeFromActor(Piece, Settings));
		}
	}

	AIvyVFXActor::SpawnIvyThroughNodes(Self, Settings, Nodes);
}

void UCard_IvyVFXEffect::CollectPropagation(ACharacterBase* Self, AActor* TargetActor, const FVector& TargetLocation, TArray<AActor*>& InOutChain) const
{
	ABoard* Board = Self ? Self->Board.Get() : nullptr;
	FTileCoord TargetTile;
	if (!Board || !Board->WorldPointToTile(TargetLocation, TargetTile))
	{
		return; // sin tablero o objetivo fuera de el: no hay piezas que buscar
	}

	ACharacterBase* TargetCharacter = Cast<ACharacterBase>(TargetActor);

	const auto Accepts = [&](ACharacterBase* Piece)
	{
		switch (PropagationTargets)
		{
		case EIvyPropagationTargets::SameTeamAsTarget:
			// Si el objetivo no es una pieza (casilla), se toma el equipo contrario al de quien juega la carta.
			return TargetCharacter ? Piece->GetTeam() == TargetCharacter->GetTeam() : Piece->GetTeam() != Self->GetTeam();
		case EIvyPropagationTargets::EnemiesOfCaster:
			return Piece->GetTeam() != Self->GetTeam();
		case EIvyPropagationTargets::AlliesOfCaster:
			return Piece->GetTeam() == Self->GetTeam();
		default:
			return true;
		}
	};

	TArray<FTileCoord> Tiles;
	Board->GetTilesInRange(TargetTile, PropagationRadius, Tiles);

	TArray<ACharacterBase*> Candidates;
	for (const FTileCoord& Tile : Tiles)
	{
		ACharacterBase* Piece = Cast<ACharacterBase>(Board->GetTileOccupant(Tile));
		if (IsValid(Piece) && Piece != Self && Piece != TargetActor && Accepts(Piece))
		{
			Candidates.AddUnique(Piece);
		}
	}

	// Cadena del vecino mas cercano empezando en el objetivo: la hiedra salta siempre a la pieza mas proxima.
	FVector Cursor = TargetLocation;
	while (Candidates.Num() > 0 && InOutChain.Num() < MaxPropagatedPieces)
	{
		int32 BestIndex = 0;
		double BestDistance = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			const double Distance = FVector::DistSquared2D(Cursor, Candidates[Index]->GetActorLocation());
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				BestIndex = Index;
			}
		}
		InOutChain.Add(Candidates[BestIndex]);
		Cursor = Candidates[BestIndex]->GetActorLocation();
		Candidates.RemoveAtSwap(BestIndex);
	}
}
