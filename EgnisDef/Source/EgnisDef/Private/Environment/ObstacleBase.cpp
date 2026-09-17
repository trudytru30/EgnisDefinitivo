#include "Environment/ObstacleBase.h"
#include "Components/HealthComponent.h"
#include "Kismet/GameplayStatics.h"

AObstacleBase::AObstacleBase()
{
	PrimaryActorTick.bCanEverTick = false;
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
}

void AObstacleBase::BeginPlay()
{
	Super::BeginPlay();

	if (!Board)
	{
		Board = Cast<ABoard>(UGameplayStatics::GetActorOfClass(GetWorld(), ABoard::StaticClass()));
	}

	if (!Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Board not found in level."), *GetName());
		return;
	}

	if (!Board->RegisterOccupant(CurrentTile, this))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Could not register occupant at Tile (%d,%d)."),
			*GetName(), CurrentTile.X, CurrentTile.Y);
		return;
	}

	FVector NewLocation = Board->TileToWorldCenter(CurrentTile);
	NewLocation.Z = GetActorLocation().Z;
	SetActorLocation(NewLocation);
}

void AObstacleBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (Board)
	{
		Board->UnregisterOccupant(CurrentTile, this);
	}
}

void AObstacleBase::ApplyDamage(float Amount)
{
	if (!bDestructible || !HealthComp)
	{
		return;
	}

	HealthComp->ApplyDelta(-Amount);

	if (HealthComp->GetCurrentHealth() <= 0.f)
	{
		HandleDestroyed();
	}
}

void AObstacleBase::HandleDestroyed()
{
	if (bDestroyHandled)
	{
		return;
	}
	bDestroyHandled = true;

	Destroy();
}