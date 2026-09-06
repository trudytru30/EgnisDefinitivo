#include "Characters/CharacterBase.h"
#include "Kismet/GameplayStatics.h" // Para localizar el Board en el nivel (ya migrado, task #3).

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
}

void ACharacterBase::BeginPlay()
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

	// Intentamos registrar a la unidad en su casilla de inicio
	const bool bRegistered = Board->RegisterOccupant(CurrentTile, this);

	if (!bRegistered)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Could not register occupant at Tile (%d,%d)."),
		*GetName(), CurrentTile.X, CurrentTile.Y);
		return;
	}

	if (bSnapToTileOnBeginPlay)
	{
		SnapToCurrentTile(false);
	}
}


void ACharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
void ACharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (Board)
	{
		Board->UnregisterOccupant(CurrentTile, this);
	}
}

// Called to bind functionality to input
void ACharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ACharacterBase::GainHealth(float AmountHealed)
{
	if (!HealthComp)
	{
		return;
	}

	HealthComp->ApplyDelta(+AmountHealed);
}

void ACharacterBase::LossHealth(float HealthToLoss)
{
	if (!HealthComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] LossHealth failed: HealthComp is null."), *GetName());
		return;
	}

	HealthComp->ApplyDelta(-HealthToLoss);

	if (HealthComp->GetCurrentHealth() <= 0)
	{
		HandleDeath();
	}
}

// Logica de muerte: desregistrarse del tablero y destruir actor
void ACharacterBase::HandleDeath()
{
	// PARCHE TEMPORAL : guarda para que esto no se ejecute dos veces. Antes,
	// HealthComponent::OnDeath() llamaba a EndPlay() por su cuenta ADEMAS de que LossHealth()
	// llamara aqui — con esa llamada ya quitada (ver HealthComponent.cpp), este HandleDeath()
	// deberia ser ya el unico camino, pero dejamos la guarda por seguridad ante cualquier otra
	// via que lo dispare dos veces. Se sustituira cuando la muerte pase a gestionarse desde GAS.
	if (bDeathHandled)
	{
		return;
	}
	bDeathHandled = true;

	if (Board)
	{
		Board->UnregisterOccupant(CurrentTile, this);
	}

	Destroy();
}


// Usado por BattleManager::RequestMove tras validar el movimiento via GridMovementComponent::canMoveToTile.
bool ACharacterBase::SetCurrentTile(const FTileCoord& NewTile)
{
	if (!Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] SetCurrentTile failed: Board is null."), *GetName());
		return false;
	}

	if (Board->IsTileOccupied(NewTile))
	{
		return false;
	}

	Board->UnregisterOccupant(CurrentTile, this);

	if (!Board->RegisterOccupant(NewTile, this))
	{
		Board->RegisterOccupant(CurrentTile, this);
		return false;
	}

	CurrentTile = NewTile;
	return true;
}

void ACharacterBase::SnapToCurrentTile(bool bKeepCurrentZ)
{
	if (!Board)
	{
		return;
	}

	FVector NewLocation = Board->TileToWorldCenter(CurrentTile);

	// bkeepCurrentZ
	if (bKeepCurrentZ)
	{
		NewLocation.Z = GetActorLocation().Z;
	}
	else
	{
		NewLocation.Z = GetActorLocation().Z;
	}

	SetActorLocation(NewLocation);
}

// Getter para obtener el equipo del personaje
int32 ACharacterBase::GetTeam()
{
	return Team;
}