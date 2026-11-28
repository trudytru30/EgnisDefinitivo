#include "Environment/ExplosiveObstacle.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/CombatLibrary.h"

AExplosiveObstacle::AExplosiveObstacle()
{
	bDestructible = true;
	InitialHealth = 10.f;
}

TArray<FTileCoord> AExplosiveObstacle::GetExplosionTiles() const
{
	TArray<FTileCoord> Tiles;
	for (int32 DX = -ExplosionRadius; DX <= ExplosionRadius; ++DX)
	{
		const int32 Rest = ExplosionRadius - FMath::Abs(DX);
		for (int32 DY = -Rest; DY <= Rest; ++DY)
		{
			if (DX == 0 && DY == 0) continue;
			const FTileCoord Tile(CurrentTile.X + DX, CurrentTile.Y + DY);
			if (!Board || Board->IsInside(Tile))
			{
				Tiles.Add(Tile);
			}
		}
	}
	return Tiles;
}

void AExplosiveObstacle::HandleDestroyed()
{
	if (bExploded) return;
	bExploded = true;

	if (Board)
	{
		// Primero se apunta quien hay: al hacer daño alguno puede morir y desaparecer del tablero
		TArray<TWeakObjectPtr<ACharacterBase>> HitCharacters;
		TArray<TWeakObjectPtr<AObstacleBase>> HitObstacles;
		for (const FTileCoord& Tile : GetExplosionTiles())
		{
			AActor* Occupant = Board->GetTileOccupant(Tile);
			if (ACharacterBase* Character = Cast<ACharacterBase>(Occupant))
			{
				HitCharacters.Add(Character);
			}
			else if (AObstacleBase* Obstacle = Cast<AObstacleBase>(Occupant))
			{
				if (bDamagesObstacles && Obstacle->bDestructible)
				{
					HitObstacles.Add(Obstacle);
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[Obstacle] %s EXPLOTA en (%d,%d): radio %d, daño %.0f, %d afectados."),
			*GetName(), CurrentTile.X, CurrentTile.Y, ExplosionRadius, ExplosionDamage,
			HitCharacters.Num() + HitObstacles.Num());

		OnExploded(CurrentTile, ExplosionRadius);

		for (const TWeakObjectPtr<ACharacterBase>& Weak : HitCharacters)
		{
			ACharacterBase* Character = Weak.Get();
			if (!IsValid(Character)) continue;
			const FString Name = Character->GetName();
			UEgnisCombatLibrary::ApplyPureDamage(Character, ExplosionDamage);
			UE_LOG(LogTemp, Log, TEXT("[Obstacle]   %s recibe %.0f de la explosion%s"),
				*Name, ExplosionDamage, Weak.IsValid() ? TEXT("") : TEXT(" (MUERTO)"));
		}

		for (const TWeakObjectPtr<AObstacleBase>& Weak : HitObstacles)
		{
			if (AObstacleBase* Obstacle = Weak.Get())
			{
				UE_LOG(LogTemp, Log, TEXT("[Obstacle]   %s recibe %.0f de la explosion"), *Obstacle->GetName(), ExplosionDamage);
				Obstacle->ApplyDamage(ExplosionDamage); // si es otro cadaver, explota en cadena
			}
		}
	}

	Super::HandleDestroyed();
}