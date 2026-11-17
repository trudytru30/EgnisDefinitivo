#include "Characters/GholMiniboss.h"
#include "Characters/EnemyAIController.h"
#include "CoreAndSystems/GameManager.h"
#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/HealthAttributeSet.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "TimerManager.h"

namespace
{
	UBattleManager* GetGholBattleManager(const UWorld* World)
	{
		if (!World) return nullptr;
		const AGameManager* GM = Cast<AGameManager>(World->GetAuthGameMode());
		const UBattleSceneManager* BSM = GM ? GM->GetBattleSceneManager() : nullptr;
		return BSM ? BSM->GetBattleManager() : nullptr;
	}

	//igual que la tabla de stats: los decimales se redondean hacia arriba
	int32 GholRoundUp(float Value)
	{
		return FMath::Max(0, FMath::CeilToInt(Value - KINDA_SMALL_NUMBER));
	}
}

AGholMiniboss::AGholMiniboss()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGholMiniboss::BeginPlay()
{
	//AEnemy::BeginPlay ya ha cargado la fila de Apero
	Super::BeginPlay();

	ApplyGholStats();

	//se entera de cuando mata a alguien (lo manda HandleDeath del que muere)
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		KillEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(TAG_Event_Unit_Killed)
			.AddUObject(this, &AGholMiniboss::HandleKillEvent);
	}
}

void AGholMiniboss::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (FGameplayEventMulticastDelegate* Delegate = ASC->GenericGameplayEventCallbacks.Find(TAG_Event_Unit_Killed))
		{
			Delegate->Remove(KillEventHandle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AGholMiniboss::ApplyGholStats()
{
	//Base = Apero (StatsRow del BP); Ghöl = Apero x multiplicadores
	NormalMaxHealth = static_cast<float>(GholRoundUp(MaxHealth * HealthMultiplier));
	NormalPower = Power * DamageMultiplier;
	NormalMoveRange = GholRoundUp(MoveRange * MoveMultiplier);

	Power = NormalPower;
	AttackDamage = NormalPower;
	MoveRange = NormalMoveRange;
	AttackRange = GholRoundUp(AttackRange * AttackRangeMultiplier);

	SetHealthValues(NormalMaxHealth, NormalMaxHealth);

	UE_LOG(LogTemp, Log, TEXT("[Ghol] %s listo: vida %.0f, poder %.1f, movimiento %d, ataque %d."),
		*GetName(), NormalMaxHealth, NormalPower, NormalMoveRange, AttackRange);
}

void AGholMiniboss::SetHealthValues(float NewMax, float NewHealth)
{
	MaxHealth = NewMax;
	Health = NewHealth;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		//Primero el maximo, para que la vida no se recorte al subirla
		ASC->SetNumericAttributeBase(UHealthAttributeSet::GetMaxHealthAttribute(), NewMax);
		ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), FMath::Clamp(NewHealth, 0.f, NewMax));
	}
}

//modo muerte (segunda barra)

void AGholMiniboss::HandleDeath()
{
	if (bFinalDeathDone) return;

	//primera barra acabada: aparece la segunda (salvo que ya no pueda revivir mas)
	if (!bInDeathMode)
	{
		const bool bCanStillRevive = MaxRevives <= 0 || ReviveCount < MaxRevives;
		if (bCanStillRevive)
		{
			GholEnterDeathMode();
			return;
		}
	}

	//segunda barra acabada a golpes
	if (IsMyTurnRunning())
	{
		//en mitad de su turno: muere cuando lo termine
		bPendingFinalDeath = true;
		return;
	}
	FinalDeath();
}

void AGholMiniboss::GholEnterDeathMode()
{
	if (bInDeathMode || bFinalDeathDone) return;

	bInDeathMode = true;
	bPendingFinalDeath = false;
	SecondBarMax = NormalMaxHealth * DeathHealthMultiplier;

	//mas fuerte mientras dura la segunda barra
	Power = NormalPower * DeathDamageMultiplier;
	AttackDamage = Power;
	MoveRange = GholRoundUp(NormalMoveRange * DeathMoveMultiplier);

	SetHealthValues(SecondBarMax, SecondBarMax);

	UE_LOG(LogTemp, Log, TEXT("[Ghol] Se queda sin vida: MODO MUERTE. Segunda barra %.0f (se gasta en %.0f s), poder %.1f, movimiento %d."),
		SecondBarMax, DeathModeSeconds, Power, MoveRange);
	OnGholDeathModeStarted(SecondBarMax, DeathModeSeconds);
}

void AGholMiniboss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInDeathMode || bPendingFinalDeath || bFinalDeathDone) return;

	const UBattleManager* BM = GetGholBattleManager(GetWorld());
	if (!BM || BM->IsBattleOver()) return;

	//mientras el jugador elige sus acciones no baja
	if (!bDrainDuringPlayerTurn && BM->IsPlayerTurn()) return;

	GholDrainDeathBar(DeltaTime);
}

void AGholMiniboss::GholDrainDeathBar(float Seconds)
{
	if (!bInDeathMode || bFinalDeathDone || Seconds <= 0.f || DeathModeSeconds <= 0.f) return;

	const float Amount = SecondBarMax / DeathModeSeconds * Seconds;
	const float NewHealth = FMath::Max(0.f, GetCurrentHealth() - Amount);
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), NewHealth);
	}
	Health = NewHealth;

	if (NewHealth <= 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("[Ghol] Se le acaba el tiempo de la segunda barra."));
		if (IsMyTurnRunning())
		{
			bPendingFinalDeath = true;
		}
		else
		{
			FinalDeath();
		}
	}
}

float AGholMiniboss::GetDeathModeSecondsLeft() const
{
	if (!bInDeathMode || SecondBarMax <= 0.f) return 0.f;
	return GetCurrentHealth() / SecondBarMax * DeathModeSeconds;
}

void AGholMiniboss::FinalDeath()
{
	if (bFinalDeathDone || !IsValid(this)) return;
	bFinalDeathDone = true;
	bInDeathMode = false;

	UE_LOG(LogTemp, Log, TEXT("[Ghol] Muere del todo (ha revivido %d veces)."), ReviveCount);
	OnGholFinalDeath();

	UWorld* World = GetWorld();
	Super::HandleDeath();

	//por si era el ultimo enemigo: que el combate compruebe si se ha ganado
	if (UBattleManager* BM = GetGholBattleManager(World))
	{
		BM->UpdateUnitsAlive();
	}
}

//revive
void AGholMiniboss::HandleKillEvent(const FGameplayEventData* Payload)
{
	if (!Payload || !bInDeathMode || bFinalDeathDone) return;

	//solo cuenta matar a una pieza del jugador
	ACharacterBase* Victim = Cast<ACharacterBase>(const_cast<AActor*>(Payload->Target.Get()));
	if (!Victim || Victim == this || Victim->GetTeam() != 0) return;

	UE_LOG(LogTemp, Log, TEXT("[Ghol] Mata a %s en modo muerte."), *Victim->GetName());
	GholRevive();
}

void AGholMiniboss::GholRevive()
{
	if (!bInDeathMode || bFinalDeathDone) return;

	bInDeathMode = false;
	bPendingFinalDeath = false;
	++ReviveCount;

	//el siguiente turno que le toque lo hace (revivido + 1) veces seguidas
	ExtraTurnsNextPhase = ReviveCount;

	Power = NormalPower;
	AttackDamage = NormalPower;
	MoveRange = NormalMoveRange;

	const float NewHealth = FMath::Max(1.f, NormalMaxHealth * ReviveHealthPercent);
	SetHealthValues(NormalMaxHealth, NewHealth);

	UE_LOG(LogTemp, Log, TEXT("[Ghol] REVIVE (vez %d) con %.0f de vida. Su proximo turno lo hara %d veces seguidas."),
		ReviveCount, NewHealth, ReviveCount + 1);
	OnGholRevived(ReviveCount, NewHealth);
}

//turnos
bool AGholMiniboss::IsMyTurnRunning() const
{
	const AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController());
	return AIController && AIController->IsTurnActive();
}

void AGholMiniboss::OnEnemyPhaseStarted()
{
	ExtraTurnsLeft = ExtraTurnsNextPhase;
	ExtraTurnsNextPhase = 0;

	if (ExtraTurnsLeft > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Ghol] Esta fase enemiga hace %d turnos seguidos."), ExtraTurnsLeft + 1);
	}
}

bool AGholMiniboss::ConsumeExtraTurn()
{
	if (bPendingFinalDeath || bFinalDeathDone || ExtraTurnsLeft <= 0) return false;

	--ExtraTurnsLeft;
	UE_LOG(LogTemp, Log, TEXT("[Ghol] Turno extra seguido (le quedan %d)."), ExtraTurnsLeft);
	OnGholExtraTurn(ExtraTurnsLeft);
	return true;
}

void AGholMiniboss::OnOwnTurnFinished()
{
	Super::OnOwnTurnFinished();

	//si se quedo sin segunda barra en mitad de su turno, muere ahora que lo ha cerrado
	if (bPendingFinalDeath)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AGholMiniboss::FinalDeath);
	}
}
