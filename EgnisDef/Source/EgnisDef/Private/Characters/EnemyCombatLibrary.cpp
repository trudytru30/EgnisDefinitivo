#include "Characters/EnemyCombatLibrary.h"
#include "Characters/Enemy.h"
#include "Characters/CharacterBase.h"
#include "Environment/ObstacleBase.h"
#include "EngineUtils.h"
#include "Characters/EnemyAIController.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/AudioManager.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Components/StatusEffectComponent.h"

namespace
{
	//GE "Add" con Set By Caller (daño negativo, escudo positivo)
	bool ApplyEnemyAmountEffect(ACharacterBase* Receiver, TSubclassOf<UGameplayEffect> Effect, float Magnitude, AActor* Instigator = nullptr)
	{
		UAbilitySystemComponent* ASC = Receiver ? Receiver->GetAbilitySystemComponent() : nullptr;
		if (!ASC || !Effect)
		{
			return false;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		if (Instigator)
		{
			Context.AddInstigator(Instigator, Instigator);
		}
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1.f, Context);
		if (!Spec.IsValid())
		{
			return false;
		}

		Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, Magnitude);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		return true;
	}
	void CollectHeroesAroundTile(UWorld* World, const FTileCoord& Center, int32 Radius, TArray<ACharacterBase*>& OutHeroes)
	{
		OutHeroes.Reset();
		if (!World)
		{
			return;
		}
		for (TActorIterator<ACharacterBase> It(World); It; ++It)
		{
			ACharacterBase* Character = *It;
			if (IsValid(Character) && Character->GetTeam() == 0 && Character->GetCurrentHealth() > 0.f
				&& UEnemyCombatLibrary::TileDistance(Center, Character->CurrentTile) <= Radius)
			{
				OutHeroes.Add(Character);
			}
		}
	}

	//Comprobaciones comunes de un golpe a un heroe: validos, no aturdido y a tiro
	bool CanStrikeHero(AEnemy* Attacker, ACharacterBase* Target)
	{
		if (!IsValid(Attacker) || !IsValid(Target) || Attacker->GetTeam() == Target->GetTeam() || Target->GetCurrentHealth() <= 0.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] Golpe sin atacante u objetivo valido."));
			return false;
		}
		if (!UEgnisCombatLibrary::CanAct(Attacker))
		{
			UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s esta aturdido: no ataca."), *Attacker->GetName());
			return false;
		}
		if (!UEnemyCombatLibrary::CanAttackTarget(Attacker, Target))
		{
			UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s no puede atacar a %s (lejos o con obstaculo en medio)."),
				*Attacker->GetName(), *Target->GetName());
			return false;
		}
		return true;
	}
}


float UEnemyCombatLibrary::RollPercent()
{
	static const float Percents[3] = { 0.33f, 0.66f, 1.0f };
	return Percents[FMath::RandRange(0, 2)];
}

float UEnemyCombatLibrary::RollValue(float BaseValue)
{
	const float Percent = RollPercent();
	const float Result = BaseValue * Percent;
	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] Tirada: %.0f%% de %.1f = %.1f"), Percent * 100.f, BaseValue, Result);
	return Result;
}

int32 UEnemyCombatLibrary::TileDistance(const FTileCoord& A, const FTileCoord& B)
{
	return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

bool UEnemyCombatLibrary::IsTileBlockingSight(const ABoard* Board, const FTileCoord& Tile)
{
	switch (Board->GetTileType(Tile))
	{
	case ETileType::Obstacle:
	case ETileType::DamagingObstacle:
	case ETileType::BreakableObstacle:
	case ETileType::ExplosiveObstacle:
		return true;
	default:
		break;
	}

	//obstaculos colocados encima de la casilla
	return Cast<AObstacleBase>(Board->GetTileOccupant(Tile)) != nullptr;
}

bool UEnemyCombatLibrary::HasLineOfSight(const ABoard* Board, const FTileCoord& From, const FTileCoord& To)
{
	if (!Board)
	{
		return false;
	}

	//recorre las casillas entre From y To, comprueba si hay obstaculos
	int32 X = From.X;
	int32 Y = From.Y;
	const int32 DX = FMath::Abs(To.X - From.X);
	const int32 DY = -FMath::Abs(To.Y - From.Y);
	const int32 StepX = From.X < To.X ? 1 : -1;
	const int32 StepY = From.Y < To.Y ? 1 : -1;
	int32 Error = DX + DY;

	while (!(X == To.X && Y == To.Y))
	{
		const int32 Error2 = 2 * Error;
		if (Error2 >= DY) { Error += DY; X += StepX; }
		if (Error2 <= DX) { Error += DX; Y += StepY; }

		
		if (X == To.X && Y == To.Y)
		{
			break;
		}
		if (IsTileBlockingSight(Board, FTileCoord(X, Y)))
		{
			return false;
		}
	}
	return true;
}

bool UEnemyCombatLibrary::CanAttackTarget(const AEnemy* Attacker, const ACharacterBase* Target)
{
	if (!IsValid(Attacker) || !IsValid(Target) || !Attacker->Board)
	{
		return false;
	}

	if (TileDistance(Attacker->CurrentTile, Target->CurrentTile) > Attacker->GetAttackRange())
	{
		return false;
	}

	return Attacker->bAttacksOverObstacles
		|| HasLineOfSight(Attacker->Board, Attacker->CurrentTile, Target->CurrentTile);
}

TArray<ACharacterBase*> UEnemyCombatLibrary::GetHeroesInAttackRange(const AEnemy* Attacker)
{
	TArray<ACharacterBase*> Result;
	if (!IsValid(Attacker) || !Attacker->GetWorld())
	{
		return Result;
	}

	for (TActorIterator<ACharacterBase> It(Attacker->GetWorld()); It; ++It)
	{
		ACharacterBase* Character = *It;
		if (IsValid(Character) && Character->GetTeam() == 0 && Character->GetCurrentHealth() > 0.f
			&& CanAttackTarget(Attacker, Character))
		{
			Result.Add(Character);
		}
	}
	return Result;
}

float UEnemyCombatLibrary::EnemyAttack(AEnemy* Attacker, ACharacterBase* Target)
{
	if (!IsValid(Attacker) || !IsValid(Target) || Attacker->GetTeam() == Target->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] Ataque sin atacante u objetivo valido."));
		return 0.f;
	}
	if (!UEgnisCombatLibrary::CanAct(Attacker))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s esta aturdido: no ataca."), *Attacker->GetName());
		return 0.f;
	}
	if (!CanAttackTarget(Attacker, Target))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s no puede atacar a %s (lejos o con obstaculo en medio)."),
			*Attacker->GetName(), *Target->GetName());
		return 0.f;
	}

	const float Multiplier = Attacker->GetDamageMultiplier();
	const float Rolled = RollValue(Attacker->GetPower()) * Multiplier;
	const float Damage = UEgnisCombatLibrary::CalculateDamage(Attacker, Target, Rolled);
	const FString TargetName = Target->GetName();

	if (!ApplyEnemyAmountEffect(Target, Attacker->AttackEffect, -Damage, Attacker))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] %s no tiene Attack Effect (GE_CardDamage) asignado."), *Attacker->GetName());
		return 0.f;
	}

	if (UAudioManager* AM = Attacker->GetGameInstance()->GetSubsystem<UAudioManager>())
	{
		AM->PlayTrigger(EEgnisAudioTrigger::EnemyAttack, nullptr, Attacker);
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s ataca a %s: %.0f de daño (bufo x%.2f). Le queda vida %.0f, escudo %.0f."),
		*Attacker->GetName(), *TargetName, Damage, Multiplier,
		IsValid(Target) ? Target->GetCurrentHealth() : 0.f,
		IsValid(Target) ? Target->GetCurrentShield() : 0.f);

	Attacker->NotifyAction(EEnemyActionType::Attack, Target, Damage);
	
	return Damage;
}

float UEnemyCombatLibrary::EnemyShield(AEnemy* Caster, ACharacterBase* Receiver)
{
	if (!IsValid(Caster))
	{
		return 0.f;
	}
	if (!IsValid(Receiver))
	{
		Receiver = Caster;
	}
	if (Caster->GetTeam() != Receiver->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] %s no puede dar escudo a alguien de otro equipo."), *Caster->GetName());
		return 0.f;
	}
	if (!UEgnisCombatLibrary::CanAct(Caster))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s esta aturdido: no pone escudo."), *Caster->GetName());
		return 0.f;
	}

	const float Amount = FMath::Max(1.f, FMath::RoundToFloat(RollValue(Caster->GetPower())));
	if (!ApplyEnemyAmountEffect(Receiver, Caster->ShieldEffect, Amount))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] %s no tiene Shield Effect (GE_CardShield) asignado."), *Caster->GetName());
		return 0.f;
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s da %.0f de escudo a %s (escudo total %.0f)."),
		*Caster->GetName(), Amount, *Receiver->GetName(), Receiver->GetCurrentShield());

	Caster->NotifyAction(EEnemyActionType::Shield, Receiver, Amount);
	
	return Amount;
}

bool UEnemyCombatLibrary::EnemyDamageBuff(AEnemy* Caster, AEnemy* Receiver, float Percent, int32 Turns)
{
	if (!IsValid(Caster))
	{
		return false;
	}
	if (!IsValid(Receiver))
	{
		Receiver = Caster;
	}
	if (!UEgnisCombatLibrary::CanAct(Caster))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s esta aturdido: no pone bufo."), *Caster->GetName());
		return false;
	}

	//Si el receptor esta en su propio turno, el final de este turno no le descuenta
	const AEnemyAIController* ReceiverAI = Cast<AEnemyAIController>(Receiver->GetController());
	Receiver->AddDamageBuff(Percent, Turns, ReceiverAI && ReceiverAI->IsTurnActive());

	Caster->NotifyAction(EEnemyActionType::DamageBuff, Receiver, Percent);
	
	return true;
}

//CURA
float UEnemyCombatLibrary::EnemyHeal(AEnemy* Caster, ACharacterBase* Receiver)
{
	if (!IsValid(Caster))
	{
		return 0.f;
	}
	if (!IsValid(Receiver))
	{
		Receiver = Caster;
	}
	if (Caster->GetTeam() != Receiver->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] %s no puede curar a alguien de otro equipo."), *Caster->GetName());
		return 0.f;
	}
	if (!UEgnisCombatLibrary::CanAct(Caster))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s esta aturdido: no cura."), *Caster->GetName());
		return 0.f;
	}

	const float Missing = Receiver->GetMaxHealth() - Receiver->GetCurrentHealth();
	if (Missing <= 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s ya tiene la vida llena."), *Receiver->GetName());
		return 0.f;
	}

	const float Rolled = FMath::Max(1.f, FMath::RoundToFloat(RollValue(Caster->GetPower())));
	const float Amount = FMath::Min(Rolled, Missing);

	//Si no tiene Heal Effect propio, usa el de ataque (GE_CardDamage con valor positivo = cura)
	const TSubclassOf<UGameplayEffect> Effect = Caster->HealEffect ? Caster->HealEffect : Caster->AttackEffect;
	if (!ApplyEnemyAmountEffect(Receiver, Effect, Amount))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] %s no tiene Heal Effect ni Attack Effect asignado."), *Caster->GetName());
		return 0.f;
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s cura %.0f a %s (vida %.0f/%.0f)."),
		*Caster->GetName(), Amount, *Receiver->GetName(), Receiver->GetCurrentHealth(), Receiver->GetMaxHealth());

	Caster->NotifyAction(EEnemyActionType::Heal, Receiver, Amount);

	return Amount;
}

ACharacterBase* UEnemyCombatLibrary::FindMostHurtAlly(AEnemy* Caster, float MaxHealthPercent, bool bIncludeSelf)
{
	if (!IsValid(Caster) || !Caster->GetWorld())
	{
		return nullptr;
	}

	ACharacterBase* Best = nullptr;
	float BestRatio = TNumericLimits<float>::Max();
	for (TActorIterator<ACharacterBase> It(Caster->GetWorld()); It; ++It)
	{
		ACharacterBase* Character = *It;
		if (!IsValid(Character) || Character->GetTeam() != Caster->GetTeam()
			|| Character->GetCurrentHealth() <= 0.f || Character->GetMaxHealth() <= 0.f)
		{
			continue;
		}
		if (!bIncludeSelf && Character == Caster)
		{
			continue;
		}
		if (TileDistance(Caster->CurrentTile, Character->CurrentTile) > Caster->GetAttackRange())
		{
			continue;
		}

		const float Ratio = Character->GetCurrentHealth() / Character->GetMaxHealth();
		if (Ratio >= MaxHealthPercent)
		{
			continue;
		}
		if (Ratio < BestRatio)
		{
			BestRatio = Ratio;
			Best = Character;
		}
	}
	return Best;
}

ACharacterBase* UEnemyCombatLibrary::FindHeroByHealth(const AEnemy* Enemy, bool bMostHealth, bool bOnlyInAttackRange)
{
	if (!IsValid(Enemy) || !Enemy->GetWorld())
	{
		return nullptr;
	}

	ACharacterBase* Best = nullptr;
	for (TActorIterator<ACharacterBase> It(Enemy->GetWorld()); It; ++It)
	{
		ACharacterBase* Character = *It;
		if (!IsValid(Character) || Character->GetTeam() != 0 || Character->GetCurrentHealth() <= 0.f)
		{
			continue;
		}
		if (bOnlyInAttackRange && !CanAttackTarget(Enemy, Character))
		{
			continue;
		}
		if (!Best
			|| (bMostHealth && Character->GetCurrentHealth() > Best->GetCurrentHealth())
			|| (!bMostHealth && Character->GetCurrentHealth() < Best->GetCurrentHealth()))
		{
			Best = Character;
		}
	}
	return Best;
}

int32 UEnemyCombatLibrary::FindBestAreaCenter(const AEnemy* Attacker, int32 Radius, FTileCoord& OutCenter)
{
	OutCenter = FTileCoord();
	if (!IsValid(Attacker) || !Attacker->Board)
	{
		return 0;
	}

	const ABoard* Board = Attacker->Board;
	TArray<FTileCoord> Candidates;
	Board->GetTilesInRange(Attacker->CurrentTile, Attacker->GetAttackRange(), Candidates);
	Candidates.Add(Attacker->CurrentTile);

	int32 BestHits = 0;
	int32 BestDistance = MAX_int32;
	TArray<ACharacterBase*> Heroes;
	for (const FTileCoord& Tile : Candidates)
	{
		if (!Attacker->bAttacksOverObstacles && !HasLineOfSight(Board, Attacker->CurrentTile, Tile))
		{
			continue;
		}

		CollectHeroesAroundTile(Attacker->GetWorld(), Tile, FMath::Max(0, Radius), Heroes);
		const int32 Distance = TileDistance(Attacker->CurrentTile, Tile);
		//Mas heroes gana; si empatan, el centro mas cercano al enemigo
		if (Heroes.Num() > BestHits || (Heroes.Num() == BestHits && BestHits > 0 && Distance < BestDistance))
		{
			BestHits = Heroes.Num();
			BestDistance = Distance;
			OutCenter = Tile;
		}
	}
	return BestHits;
}

float UEnemyCombatLibrary::EnemyAreaAttack(AEnemy* Attacker, const FTileCoord& Center, int32 Radius, bool bSplitDamage)
{
	if (!IsValid(Attacker) || !Attacker->Board)
	{
		return 0.f;
	}
	if (!UEgnisCombatLibrary::CanAct(Attacker))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s esta aturdido: no ataca."), *Attacker->GetName());
		return 0.f;
	}

	const ABoard* Board = Attacker->Board;
	if (!Board->IsInside(Center) || TileDistance(Attacker->CurrentTile, Center) > Attacker->GetAttackRange()
		|| (!Attacker->bAttacksOverObstacles && !HasLineOfSight(Board, Attacker->CurrentTile, Center)))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s no puede lanzar el area a (%d,%d) (fuera de rango, del tablero o con obstaculo en medio)."),
			*Attacker->GetName(), Center.X, Center.Y);
		return 0.f;
	}

	TArray<ACharacterBase*> Targets;
	CollectHeroesAroundTile(Attacker->GetWorld(), Center, FMath::Max(0, Radius), Targets);
	if (Targets.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] El area de %s en (%d,%d) no pilla a ningun heroe."),
			*Attacker->GetName(), Center.X, Center.Y);
		return 0.f;
	}

	const float Multiplier = Attacker->GetDamageMultiplier();
	const float Rolled = RollValue(Attacker->GetPower()) * Multiplier;
	const float PerTarget = bSplitDamage ? Rolled / Targets.Num() : Rolled;

	float Total = 0.f;
	for (ACharacterBase* Target : Targets)
	{
		if (!IsValid(Target))
		{
			continue;
		}
		const float Damage = UEgnisCombatLibrary::CalculateDamage(Attacker, Target, PerTarget);
		const FString TargetName = Target->GetName();
		if (!ApplyEnemyAmountEffect(Target, Attacker->AttackEffect, -Damage, Attacker))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] %s no tiene Attack Effect (GE_CardDamage) asignado."), *Attacker->GetName());
			return Total;
		}
		Total += Damage;
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat]   %s recibe %.0f (vida %.0f, escudo %.0f)."), *TargetName, Damage,
			IsValid(Target) ? Target->GetCurrentHealth() : 0.f, IsValid(Target) ? Target->GetCurrentShield() : 0.f);
	}

	if (UAudioManager* AM = Attacker->GetGameInstance()->GetSubsystem<UAudioManager>())
	{
		AM->PlayTrigger(EEgnisAudioTrigger::EnemyAttack, nullptr, Attacker);
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s ataque en area en (%d,%d) radio %d%s: %d heroes, %.0f de daño total (bufo x%.2f)."),
		*Attacker->GetName(), Center.X, Center.Y, Radius, bSplitDamage ? TEXT(" repartido") : TEXT(""),
		Targets.Num(), Total, Multiplier);

	Attacker->NotifyAction(EEnemyActionType::AreaAttack, nullptr, Total);
	
	return Total;
}

bool UEnemyCombatLibrary::EnemyExecute(AEnemy* Attacker, ACharacterBase* Target, float ExecuteBelowPercent)
{
	if (!CanStrikeHero(Attacker, Target))
	{
		return false;
	}
	const float Ratio = Target->GetMaxHealth() > 0.f ? Target->GetCurrentHealth() / Target->GetMaxHealth() : 1.f;
	if (Ratio >= ExecuteBelowPercent)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s no puede ejecutar a %s: tiene el %.0f%% de vida (hace falta menos del %.0f%%)."),
			*Attacker->GetName(), *Target->GetName(), Ratio * 100.f, ExecuteBelowPercent * 100.f);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s EJECUTA a %s (tenia %.0f de vida, %.0f%%)."),
		*Attacker->GetName(), *Target->GetName(), Target->GetCurrentHealth(), Ratio * 100.f);
	UEgnisCombatLibrary::ApplyPureDamage(Target, Target->GetCurrentHealth());

	if (UAudioManager* AM = Attacker->GetGameInstance()->GetSubsystem<UAudioManager>())
	{
		AM->PlayTrigger(EEgnisAudioTrigger::EnemyAttack, nullptr, Attacker);
	}

	Attacker->NotifyAction(EEnemyActionType::Execute, Target, 0.f);
	
	return true;
}

float UEnemyCombatLibrary::EnemyHalveHealth(AEnemy* Attacker, ACharacterBase* Target)
{
	if (!CanStrikeHero(Attacker, Target))
	{
		return 0.f;
	}
	const float Amount = FMath::Max(1.f, FMath::RoundToFloat(Target->GetCurrentHealth() * 0.5f));
	const FString TargetName = Target->GetName();
	UEgnisCombatLibrary::ApplyPureDamage(Target, Amount);

	if (UAudioManager* AM = Attacker->GetGameInstance()->GetSubsystem<UAudioManager>())
	{
		AM->PlayTrigger(EEgnisAudioTrigger::EnemyAttack, nullptr, Attacker);
	}
	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s le quita la mitad de la vida a %s: -%.0f (le queda %.0f)."),
		*Attacker->GetName(), *TargetName, Amount, IsValid(Target) ? Target->GetCurrentHealth() : 0.f);

	Attacker->NotifyAction(EEnemyActionType::HalveHealth, Target, Amount);
	return Amount;
}

bool UEnemyCombatLibrary::EnemyApplyBleed(AEnemy* Attacker, ACharacterBase* Target, int32 Turns)
{
	if (!CanStrikeHero(Attacker, Target) || !Target->StatusComp)
	{
		return false;
	}
	const bool bApplied = Target->StatusComp->ApplyStatus(EStatusType::Status_Bleed, FMath::Max(1, Turns), Attacker);
	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s pone sangrado a %s durante %d turnos."),
		*Attacker->GetName(), *Target->GetName(), FMath::Max(1, Turns));

	Attacker->NotifyAction(EEnemyActionType::Bleed, Target, Turns);
	
	return bApplied;
}

EEnemyStrikeResult UEnemyCombatLibrary::EnemyExecutionStrike(AEnemy* Attacker, ACharacterBase* Target, float ExecuteBelowPercent, int32 BleedTurns)
{
	if (!CanStrikeHero(Attacker, Target))
	{
		return EEnemyStrikeResult::None;
	}

	const float Ratio = Target->GetMaxHealth() > 0.f ? Target->GetCurrentHealth() / Target->GetMaxHealth() : 1.f;
	if (Ratio < ExecuteBelowPercent)
	{
		return EnemyExecute(Attacker, Target, ExecuteBelowPercent) ? EEnemyStrikeResult::Executed : EEnemyStrikeResult::None;
	}
	if (Target->GetCurrentHealth() > Attacker->GetCurrentHealth())
	{
		return EnemyApplyBleed(Attacker, Target, BleedTurns) ? EEnemyStrikeResult::Bled : EEnemyStrikeResult::None;
	}
	return EnemyHalveHealth(Attacker, Target) > 0.f ? EEnemyStrikeResult::HalvedHealth : EEnemyStrikeResult::None;
}



void UEnemyCombatLibrary::BuildReachableMap(const AEnemy* Enemy, TMap<FTileCoord, int32>& OutSteps)
{
	OutSteps.Reset();
	if (!IsValid(Enemy) || !Enemy->Board)
	{
		return;
	}

	const ABoard* Board = Enemy->Board;
	const int32 MaxSteps = UEgnisCombatLibrary::CanMove(Enemy) ? Enemy->GetMoveRange() : 0;
	static const FTileCoord Dirs[4] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

	//Busqueda por anchura: primero todas las casillas a 1 paso, luego a 2...
	TArray<FTileCoord> Frontier;
	Frontier.Add(Enemy->CurrentTile);
	OutSteps.Add(Enemy->CurrentTile, 0);

	for (int32 Index = 0; Index < Frontier.Num(); ++Index)
	{
		const FTileCoord Tile = Frontier[Index];
		const int32 Steps = OutSteps[Tile];
		if (Steps >= MaxSteps)
		{
			continue;
		}

		for (const FTileCoord& Dir : Dirs)
		{
			const FTileCoord Next(Tile.X + Dir.X, Tile.Y + Dir.Y);
			//IsTileOccupied ya cuenta fuera del tablero y huecos (Void) como ocupadas
			if (OutSteps.Contains(Next) || Board->IsTileOccupied(Next) || IsTileBlockingSight(Board, Next))
			{
				continue;
			}
			OutSteps.Add(Next, Steps + 1);
			Frontier.Add(Next);
		}
	}
}

TArray<FTileCoord> UEnemyCombatLibrary::GetReachableTiles(const AEnemy* Enemy)
{
	TMap<FTileCoord, int32> Steps;
	BuildReachableMap(Enemy, Steps);

	TArray<FTileCoord> Result;
	for (const TPair<FTileCoord, int32>& Pair : Steps)
	{
		if (Pair.Value > 0)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

bool UEnemyCombatLibrary::EnemyMoveToTile(AEnemy* Enemy, const FTileCoord& Target)
{
	if (!IsValid(Enemy) || !Enemy->Board)
	{
		return false;
	}
	if (!UEgnisCombatLibrary::CanMove(Enemy))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyMove] %s esta paralizado: no se mueve."), *Enemy->GetName());
		return false;
	}
	if (Target == Enemy->CurrentTile)
	{
		return true;
	}

	TMap<FTileCoord, int32> Steps;
	BuildReachableMap(Enemy, Steps);
	const int32* StepsToTarget = Steps.Find(Target);
	if (!StepsToTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyMove] %s no puede llegar a (%d,%d)."), *Enemy->GetName(), Target.X, Target.Y);
		return false;
	}

	const FTileCoord From = Enemy->CurrentTile;
	if (!Enemy->SetCurrentTile(Target))
	{
		return false;
	}
	
	if (!IsValid(Enemy)) return true; // murio al pisar fuego
	
	Enemy->SnapToCurrentTile();

	if (UAudioManager* AM = Enemy->GetGameInstance()->GetSubsystem<UAudioManager>())
	{
		AM->PlayTrigger(EEgnisAudioTrigger::EnemyMove, nullptr, Enemy);
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyMove] %s se mueve de (%d,%d) a (%d,%d): %d pasos de %d."),
		*Enemy->GetName(), From.X, From.Y, Target.X, Target.Y, *StepsToTarget, Enemy->GetMoveRange());
	
	Enemy->NotifyMoved(From, Target, *StepsToTarget);

	return true;
}

bool UEnemyCombatLibrary::FindTileTowards(const AEnemy* Enemy, const ACharacterBase* Target, FTileCoord& OutTile)
{
	if (!IsValid(Enemy) || !IsValid(Target) || !Enemy->Board)
	{
		return false;
	}

	TMap<FTileCoord, int32> Steps;
	BuildReachableMap(Enemy, Steps);

	bool bBestCanAttack = false;
	int32 BestDistance = MAX_int32;
	int32 BestSteps = MAX_int32;
	OutTile = Enemy->CurrentTile;

	for (const TPair<FTileCoord, int32>& Pair : Steps)
	{
		const FTileCoord& Tile = Pair.Key;
		const int32 Distance = TileDistance(Tile, Target->CurrentTile);
		const bool bCanAttack = Distance <= Enemy->GetAttackRange()
			&& (Enemy->bAttacksOverObstacles || HasLineOfSight(Enemy->Board, Tile, Target->CurrentTile));

		//Prioridad: 1) poder atacar  2) estar mas cerca  3) andar menos
		bool bBetter;
		if (bCanAttack != bBestCanAttack)
		{
			bBetter = bCanAttack;
		}
		else if (bCanAttack)
		{
			bBetter = Pair.Value < BestSteps;
		}
		else
		{
			bBetter = Distance < BestDistance || (Distance == BestDistance && Pair.Value < BestSteps);
		}

		if (bBetter)
		{
			bBestCanAttack = bCanAttack;
			BestDistance = Distance;
			BestSteps = Pair.Value;
			OutTile = Tile;
		}
	}
	return true;
}

bool UEnemyCombatLibrary::EnemyMoveTowards(AEnemy* Enemy, ACharacterBase* Target)
{
	FTileCoord Tile;
	if (!FindTileTowards(Enemy, Target, Tile))
	{
		return false;
	}
	if (Tile == Enemy->CurrentTile)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyMove] %s se queda en (%d,%d) (ya esta a tiro de %s o no puede acercarse mas)."),
			*Enemy->GetName(), Tile.X, Tile.Y, *Target->GetName());
		return true;
	}
	return EnemyMoveToTile(Enemy, Tile);
}

bool UEnemyCombatLibrary::EnemyMoveToLocation(AEnemy* Enemy, const FVector& Location)
{
	if (!IsValid(Enemy) || !Enemy->Board)
	{
		return false;
	}

	FTileCoord Tile;
	if (!Enemy->Board->WorldPointToTile(Location, Tile))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyMove] %s: la ubicacion (%s) esta fuera del tablero."), *Enemy->GetName(), *Location.ToString());
		return false;
	}
	return EnemyMoveToTile(Enemy, Tile);
}

bool UEnemyCombatLibrary::GetHeroesCenterTile(const AEnemy* Enemy, FTileCoord& OutCenter)
{
	OutCenter = FTileCoord();
	if (!IsValid(Enemy) || !Enemy->GetWorld())
	{
		return false;
	}

	int32 SumX = 0;
	int32 SumY = 0;
	int32 Count = 0;
	for (TActorIterator<ACharacterBase> It(Enemy->GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetTeam() == 0 && It->GetCurrentHealth() > 0.f)
		{
			SumX += It->CurrentTile.X;
			SumY += It->CurrentTile.Y;
			++Count;
		}
	}
	if (Count == 0)
	{
		return false;
	}

	OutCenter = FTileCoord(FMath::RoundToInt(static_cast<float>(SumX) / Count), FMath::RoundToInt(static_cast<float>(SumY) / Count));
	return true;
}

bool UEnemyCombatLibrary::FindTileTowardsHeroesCenter(const AEnemy* Enemy, FTileCoord& OutTile)
{
	OutTile = IsValid(Enemy) ? Enemy->CurrentTile : FTileCoord();
	FTileCoord Center;
	if (!GetHeroesCenterTile(Enemy, Center))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyMove] No hay heroes: no hay centro al que ir."));
		return false;
	}

	TMap<FTileCoord, int32> Steps;
	BuildReachableMap(Enemy, Steps);

	int32 BestDistance = MAX_int32;
	int32 BestSteps = MAX_int32;
	for (const TPair<FTileCoord, int32>& Pair : Steps)
	{
		const int32 Distance = TileDistance(Pair.Key, Center);
		if (Distance < BestDistance || (Distance == BestDistance && Pair.Value < BestSteps))
		{
			BestDistance = Distance;
			BestSteps = Pair.Value;
			OutTile = Pair.Key;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyMove] %s: centro de los heroes en (%d,%d); mejor casilla (%d,%d), a %d del centro."),
		*Enemy->GetName(), Center.X, Center.Y, OutTile.X, OutTile.Y, BestDistance);
	return true;
}

bool UEnemyCombatLibrary::EnemyMoveToHeroesCenter(AEnemy* Enemy)
{
	FTileCoord Tile;
	if (!FindTileTowardsHeroesCenter(Enemy, Tile))
	{
		return false;
	}
	if (Tile == Enemy->CurrentTile)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyMove] %s se queda en (%d,%d): es lo mas cerca del centro que puede estar."),
			*Enemy->GetName(), Tile.X, Tile.Y);
		return true;
	}
	return EnemyMoveToTile(Enemy, Tile);
}

float UEnemyCombatLibrary::GetHealthPlusShieldPercent(const ACharacterBase* Character)
{
	if (!IsValid(Character) || Character->GetMaxHealth() <= 0.f)
	{
		return 0.f;
	}
	return (Character->GetCurrentHealth() + Character->GetCurrentShield()) / Character->GetMaxHealth() * 100.f;
}

float UEnemyCombatLibrary::EnemyLoseShieldPercent(AEnemy* Enemy, float Percent)
{
	if (!IsValid(Enemy))
	{
		return 0.f;
	}
	const float Amount = FMath::RoundToFloat(Enemy->GetCurrentShield() * FMath::Clamp(Percent, 0.f, 100.f) / 100.f);
	if (Amount <= 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s no tiene escudo que perder."), *Enemy->GetName());
		return 0.f;
	}
	if (!ApplyEnemyAmountEffect(Enemy, Enemy->ShieldEffect, -Amount))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyCombat] %s no tiene Shield Effect (GE_CardShield) asignado."), *Enemy->GetName());
		return 0.f;
	}
	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s pierde %.0f de escudo (%.0f%%): le queda %.0f. Vida+escudo = %.0f%%."),
		*Enemy->GetName(), Amount, Percent, Enemy->GetCurrentShield(), GetHealthPlusShieldPercent(Enemy));

	Enemy->NotifyAction(EEnemyActionType::LoseShield, Enemy, Amount);
	
	return Amount;
}

int32 UEnemyCombatLibrary::EnemyTaunt(AEnemy* Caster, int32 Turns, int32 MaxDistance)
{
	if (!IsValid(Caster) || !Caster->GetWorld())
	{
		return 0;
	}
	if (!UEgnisCombatLibrary::CanAct(Caster))
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s esta aturdido: no provoca."), *Caster->GetName());
		return 0;
	}

	int32 Count = 0;
	for (TActorIterator<ACharacterBase> It(Caster->GetWorld()); It; ++It)
	{
		ACharacterBase* Hero = *It;
		if (!IsValid(Hero) || Hero->GetTeam() != 0 || Hero->GetCurrentHealth() <= 0.f || !Hero->StatusComp)
		{
			continue;
		}
		if (MaxDistance > 0 && TileDistance(Caster->CurrentTile, Hero->CurrentTile) > MaxDistance)
		{
			continue;
		}
		if (Hero->StatusComp->ApplyStatus(EStatusType::Status_Taunt, FMath::Max(1, Turns), Caster))
		{
			++Count;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[EnemyCombat] %s provoca a %d heroes durante %d turno(s)%s."), *Caster->GetName(), Count,
		FMath::Max(1, Turns), MaxDistance > 0 ? *FString::Printf(TEXT(" (a %d casillas o menos)"), MaxDistance) : TEXT(""));

	Caster->NotifyAction(EEnemyActionType::Taunt, nullptr, Count);
	return Count;
}