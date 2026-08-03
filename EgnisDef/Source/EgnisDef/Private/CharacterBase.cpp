#include "CharacterBase.h"
// TODO: Descomentar cuando se migre ABoard (se usaba para localizar el Board en el nivel)
// #include "Kismet/GameplayStatics.h"

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	MovementComp = CreateDefaultSubobject<UGridMovementComponent>(TEXT("MovementComp"));
}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Todo este bloque depende de ABoard, que de momento no existe en este proyecto.
	// Comentado para evitar errores de compilacion. Descomentar al migrar Board.

	// if(!Board)
	// {
	// 	Board = Cast<ABoard>(UGameplayStatics::GetActorOfClass(GetWorld(), ABoard::StaticClass()));
	// }

	// if (!Board)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("[%s] Board not found in level."), *GetName());
	// 	return;
	// }

	// //Intentamos registrar a la unidad en su casilla de inicio
	// const bool bRegistered = Board->RegisterOccupant(CurrentTile, this);

	// if (!bRegistered)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("[%s] Could not register occupant at Tile (%d,%d)."),
	// 	*GetName(), CurrentTile.X, CurrentTile.Y);
	// 	return;
	// }
	// SnapToCurrentTile(false);
}


void ACharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
void ACharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Depende de ABoard, comentado de momento. Descomentar al migrar Board.
	// if (Board)
	// {
	// 	Board->UnregisterOccupant(CurrentTile, this);
	// }
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
	// Depende de ABoard, comentado de momento. Descomentar al migrar Board.
	// if (Board)
	// {
	// 	Board->UnregisterOccupant(CurrentTile, this);
	// }

	Destroy();
}


// De momento no se usa: depende por completo de ABoard / FTileCoord.
// bool ACharacterBase::SetCurrentTile(const FTileCoord& NewTile)
// {
// 	if (!Board)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("[%s] SetCurrentTile failed: Board is null."), *GetName());
// 		return false;
// 	}
//
// 	if (Board->IsTileOccupied(NewTile))
// 	{
// 		return false;
// 	}
//
// 	Board->UnregisterOccupant(CurrentTile,this);
//
// 	if (!Board->RegisterOccupant(NewTile, this))
// 	{
// 		Board->RegisterOccupant(CurrentTile, this);
// 		return false;
// 	}
//
// 	CurrentTile = NewTile;
// 	return true;
// }

// De momento no se usa: depende por completo de ABoard.
// void ACharacterBase::SnapToCurrentTile(bool bKeepCurrentZ)
// {
// 	if (!Board)
// 	{
// 		return;
// 	}
//
// 	FVector NewLocation = Board->TileToWorldCenter(CurrentTile);
//
// 	if (bKeepCurrentZ)
// 	{
// 		NewLocation.Z = GetActorLocation().Z;
// 	}
// 	else
// 	{
// 		NewLocation.Z = GetActorLocation().Z;
// 	}
//
// 	SetActorLocation(NewLocation);
// }

// Getter para obtener el equipo del personaje
int32 ACharacterBase::GetTeam()
{
	return Team;
}