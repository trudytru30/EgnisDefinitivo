#include "Characters/CharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h" // Para localizar el Board en el nivel
#include "AbilitySystemComponent.h" // Necesario para CreateDefaultSubobject<UAbilitySystemComponent>
#include "Components/StatusEffectComponent.h"
#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Abilities/EgnisAbilityLibrary.h"
#include "CoreAndSystems/EgnisGameplayTags.h"

ACharacterBase::ACharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// Igual patrón que HealthComp/HealthAttributeSet: se crea en construcción para que exista desde el primer frame
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	HealthAttributeSet = CreateDefaultSubobject<UHealthAttributeSet>(TEXT("HealthAttributeSet"));
	StatusComp = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusComp"));
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
	
	// Aplicar los valores por defecto editados desde el Blueprint/Detalles
	if (HealthAttributeSet)
	{
		HealthAttributeSet->InitMaxHealth(MaxHealth);
		HealthAttributeSet->InitHealth(Health);
		HealthAttributeSet->InitShield(InitialShield);
	}

	if (!Board)
	{
		Board = Cast<ABoard>(UGameplayStatics::GetActorOfClass(GetWorld(), ABoard::StaticClass()));
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

	AActor* Killer = LastDamageInstigator.Get();
	UEgnisAbilityLibrary::SendEvent(Killer, TAG_Event_Unit_Killed, Killer, this, 0.f, nullptr, FGameplayTagContainer());
	UEgnisAbilityLibrary::SendEventToAllCharacters(this, TAG_Event_Unit_Died, Killer, this, 0.f, this);

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

	const FTileCoord LeftTile = CurrentTile;
	PreviousTile = LeftTile;
	CurrentTile = NewTile;

	const int32 Distance = FMath::Max(FMath::Abs(NewTile.X - LeftTile.X), FMath::Abs(NewTile.Y - LeftTile.Y));
	UEgnisAbilityLibrary::SendEvent(this, TAG_Event_Unit_Moved, this, nullptr, Distance, nullptr, FGameplayTagContainer());

	// Rastro de la Quemadura: si esta quemado, deja en llamas la casilla que acaba de dejar
	if (StatusComp && StatusComp->HasStatus(EStatusType::Status_Burn))
	{
		Board->AddFireTile(LeftTile, StatusComp->BurnTrailTurns, StatusComp->BurnTrailDamage);
		UE_LOG(LogTemp, Log, TEXT("[Fire] %s (quemado) deja rastro en (%d,%d): %d turnos, %.0f de daño"),
			*GetName(), LeftTile.X, LeftTile.Y, StatusComp->BurnTrailTurns, StatusComp->BurnTrailDamage);
	}

	// Pisar una casilla en llamas: daño puro. Puede matar y destruir a este personaje,
	// por eso va lo ultimo y quien llame a SetCurrentTile debe comprobar IsValid despues.
	FFireTile Fire;
	if (Board->GetFireTile(NewTile, Fire))
	{
		const float HealthBefore = GetCurrentHealth();
		UEgnisCombatLibrary::ApplyPureDamage(this, Fire.Damage);
		// Ojo: si murió, 'this' ya está destruido (HandleDeath → Destroy). Por eso solo uso el nombre guardado antes.
		UE_LOG(LogTemp, Log, TEXT("[Fire] %s pisa casilla en llamas (%d,%d): daño puro %.0f, vida %.0f -> %.0f%s"),
			*GetName(), NewTile.X, NewTile.Y, Fire.Damage, HealthBefore,
			IsValid(this) ? GetCurrentHealth() : 0.f,
			IsValid(this) ? TEXT("") : TEXT(" (MUERTO)"));
	}
	
	return true;
}
void ACharacterBase::SnapToCurrentTile(bool bKeepCurrentZ)
{
	if (!Board) return;

	FVector NewLocation = GetStandLocationForTile(CurrentTile);

	if (bKeepCurrentZ)
	{
		NewLocation.Z = GetActorLocation().Z;
	}

	SetActorLocation(NewLocation);
}

FVector ACharacterBase::GetStandLocationForTile(const FTileCoord& Tile) const
{
	if (!Board) return GetActorLocation();

	FVector Location = Board->TileToWorldCenter(Tile);
	Location.Z += TileZOffset;
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Location.Z += Capsule->GetScaledCapsuleHalfHeight();
	}
	return Location;
}


float ACharacterBase::GetCurrentHealth() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetHealth() : 0.f;
}

float ACharacterBase::GetMaxHealth() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetMaxHealth() : 0.f;
}

float ACharacterBase::GetCurrentShield() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetShield() : 0.f;
}


// Getter para obtener el equipo del personaje
int32 ACharacterBase::GetTeam()
{
	return Team;
}