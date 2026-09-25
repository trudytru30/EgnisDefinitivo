#include "Characters/CharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h" // Para localizar el Board en el nivel
#include "AbilitySystemComponent.h" // Necesario para CreateDefaultSubobject<UAbilitySystemComponent>

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthAttributeSet = CreateDefaultSubobject<UHealthAttributeSet>(TEXT("HealthAttributeSet"));
	// Igual patrón que HealthComp/HealthAttributeSet: se crea en construcción para que exista desde el primer frame
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// InitAbilityActorInfo(Owner, Avatar) le dice al ASC "quién es el dueño lógico" y "quién
		// es el cuerpo físico en el mundo" de las abilities. Sin este proyecto tener PlayerState
		// propio, lo más simple es pasar 'this' en ambos. Es obligatorio: sin esta llamada el
		// ASC existe como componente pero no funciona
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (!Board)
	{
		Board = Cast<ABoard>(UGameplayStatics::GetActorOfClass(GetWorld(), ABoard::StaticClass()));
	}

	if (!Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Board not found in level."), *GetName());
		return;
	}

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

// Logica de muerte: desregistrarse del tablero y destruir actor
void ACharacterBase::HandleDeath()
{
	// Guarda para que esto no se ejecute dos veces: HandleDeath() puede dispararse desde varios
	// sitios (PostGameplayEffectExecute de HealthAttributeSet al morir por daño real, o una
	// llamada directa vía debug/consola), y sin esta guarda una segunda llamada intentaria
	// desregistrar y destruir un actor que ya no está en el tablero.
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
	if (!Board) return;

	FVector NewLocation = Board->TileToWorldCenter(CurrentTile);

	if (bKeepCurrentZ)
	{
		NewLocation.Z = GetActorLocation().Z;
	}
	else
	{
		float BaseOffset = TileZOffset;
		if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			BaseOffset += Capsule->GetScaledCapsuleHalfHeight();
		}
		NewLocation.Z += BaseOffset;
	}

	SetActorLocation(NewLocation);
}


float ACharacterBase::GetCurrentHealth() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetHealth() : 0.f;
}

float ACharacterBase::GetMaxHealth() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetMaxHealth() : 0.f;
}


// Getter para obtener el equipo del personaje
int32 ACharacterBase::GetTeam()
{
	return Team;
}