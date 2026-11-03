#include "Environment/ObstacleBase.h"
#include "Components/HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "CoreAndSystems/HealthAttributeSet.h"

AObstacleBase::AObstacleBase()
{
	PrimaryActorTick.bCanEverTick = false;
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	HealthAttributeSet = CreateDefaultSubobject<UHealthAttributeSet>(TEXT("HealthAttributeSet"));
}

void AObstacleBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (HealthAttributeSet)
	{
		// InitialHealth a 0 = obstaculo antiguo: usa la vida que tenga puesta su HealthComp
		float StartHealth = InitialHealth;
		if (StartHealth <= 0.f && HealthComp)
		{
			StartHealth = HealthComp->GetMaxHealth();
		}
		StartHealth = FMath::Max(1.f, StartHealth);

		HealthAttributeSet->InitMaxHealth(StartHealth);
		HealthAttributeSet->InitHealth(StartHealth);
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
	if (!bDestructible || !AbilitySystemComponent || !HealthAttributeSet)
	{
		return;
	}

	// SetNumericAttributeBase no pasa por PostGameplayEffectExecute, asi que la destruccion se comprueba aqui
	const float NewHealth = FMath::Max(0.f, HealthAttributeSet->GetHealth() - Amount);
	AbilitySystemComponent->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), NewHealth);

	UE_LOG(LogTemp, Log, TEXT("Obstaculo %s: %.1f de daño, vida %.1f/%.1f"),
		*GetName(), Amount, NewHealth, HealthAttributeSet->GetMaxHealth());

	if (NewHealth <= 0.f)
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

float AObstacleBase::GetCurrentHealth() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetHealth() : 0.f;
}

float AObstacleBase::GetMaxHealth() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetMaxHealth() : 0.f;
}