#include "Characters/Enemy.h"
#include "CoreAndSystems/AudioManager.h"
#include "Cards/ActionDataAsset.h"
#include "Characters/EnemyArcheTypeDataAsset.h"
// FIX respecto al original: habia un include de "EntitySystem/MovieSceneEntitySystemRunner.h"
// (sistema de Sequencer/MovieScene) que no se usa en ningun sitio de este archivo. Aparte de ser
// codigo muerto, es una dependencia de modulo que puede no estar en el Build.cs del proyecto
// nuevo y dar error de enlazado sin necesidad. Se quita.
#include "Kismet/GameplayStatics.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	Team = 1;
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	if (!ArchetypeData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Enemy has no ArchetypeData assigned (DA_Enemy_Warrior / DA_Enemy_Healer)."),
			*GetName());
		return;
	}
	for (const UActionDataAsset* Action : ArchetypeData->Actions)
	{
		if (!Action) continue;

		const FString ActionIdStr =
			(Action->ActionId == EActionId::Attack) ? TEXT("Attack") : TEXT("Heal");
	}

	if (bDebugMovementOnBeginPlay)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle,this,&AEnemy::MoveTowardClosesPlayer,0.5f,false);
	}
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AEnemy::MoveTowardClosesPlayer()
{
	if (!Board)
	{
		return;
	}

	ACharacterBase* Target = FindClosestPlayer();
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] MoveTowardClosestPlayer_2Tiles: No player found"), *GetName());
		return;
	}

	const FTileCoord Start = CurrentTile;
	const FTileCoord Goal = Target->CurrentTile;

	// Candidatas: 4 direcciones rectas
	const FIntPoint Dirs[4] = {
		FIntPoint( 1,  0), // derecha
		FIntPoint(-1,  0), // izquierda
		FIntPoint( 0,  1), // arriba
		FIntPoint( 0, -1)  // abajo
	};

	bool bFoundDir = false;
	FIntPoint BestDir (0,0);
	int32 BestDist = ManhattanDistance(Start,Goal);

	// 1) Elegir dirección óptima (según la casilla final tras 2 pasos)

	for (const FIntPoint& Dir : Dirs)
	{
		const FTileCoord Step1(Start.X + Dir.X, Start.Y + Dir.Y);
		const FTileCoord Step2(Start.X + 2 * Dir.X, Start.Y + 2 * Dir.Y);

		//Debe poder dar 2 pasos
		if (!Board->IsInside(Step1) || !Board->IsInside(Step2))
		{
			continue;
		}

		//Deben de estar libres los 2 pasos
		if (Board->IsTileOccupied(Step1) || Board->IsTileOccupied(Step2))
		{
			continue;
		}

		const int32 DistToGoal = ManhattanDistance(Step2, Goal);
		if (DistToGoal < BestDist)
		{
			BestDist = DistToGoal;
			BestDir = Dir;
			bFoundDir = true;
		}
	}
	if (!bFoundDir)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] No tenemos camino valido  o no nos acercar al enemigo."), *GetName());
		return;
	}

	//Ejecutar Paso 1
	const FTileCoord FirstStep(Start.X + BestDir.X, Start.Y + BestDir.Y);

	if (!SetCurrentTile(FirstStep))
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Failed to move to Step1."), *GetName());
		return;
	}

	SetActorLocation(Board->TileToWorldCenter(CurrentTile));

	//Re-comprobar y ejecutar paso 2 (MISMA dirección del origen)
	const FTileCoord SecondStep(Start.X + 2 * BestDir.X, Start.Y + 2 * BestDir.Y);


	// SecondStep está definido respecto al Start (origen), por la regla de “misma dirección originaria”
	if (!Board->IsInside(SecondStep) || Board->IsTileOccupied(SecondStep))
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Step2 blocked after Step1. Stops at (%d,%d)."),
			*GetName(), CurrentTile.X, CurrentTile.Y);

		SetActorLocation(Board->TileToWorldCenter(CurrentTile));
		if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayLandSound(0);
		return;
	}
	if (!SetCurrentTile(SecondStep))
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Failed to move to Step2. Stops at (%d,%d)."),
			*GetName(), CurrentTile.X, CurrentTile.Y);

		SetActorLocation(Board->TileToWorldCenter(CurrentTile));
		if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayLandSound(0);
		return;
	}

	SetActorLocation(Board->TileToWorldCenter(CurrentTile));

	UE_LOG(LogTemp, Log, TEXT("[%s] Moved 2 tiles straight to (%d,%d) towards player (%d,%d)."),
		*GetName(), CurrentTile.X, CurrentTile.Y, Goal.X, Goal.Y);
	if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayLandSound(0);
}

void AEnemy::MakeAction()
{
	if (!ArchetypeData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] MakeAction: ArchetypeData is null."), *GetName());
		return;
	}

	ACharacterBase* Target = nullptr;
	int32 Range = 0;
	//Intentar curar si tiene Heal
	if (const UActionDataAsset* HealAction = GetActionById(EActionId::Heal))
	{
		Target = FindBestTarget(HealAction->Range, 1, 1);
		Range = HealAction->Range;
	}

	if (Target)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Heal -> %s (Range=%d) Amount=%.1f"),
			*GetName(), *Target->GetName(), Range, HealAmount);

		Target->GainHealth(HealAmount);
		return;
	}

	// Si no cura, ataca
	const UActionDataAsset* AttackAction = GetActionById(EActionId::Attack);
	if (!AttackAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] MakeAction: No Attack action available."), *GetName());
		return;
	}

	Target = FindBestTarget(AttackAction->Range, 2, 0);
	Range = AttackAction->Range;

	if (Target)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Attack -> %s (Range=%d) Damage=%.1f"),
			*GetName(), *Target->GetName(), Range, AttackDamage);

		Target->LossHealth(AttackDamage);
		if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayAttackSound(4);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] MakeAction: No valid heal/attack targets in range."), *GetName());
}


ACharacterBase* AEnemy::FindClosestPlayer() const
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacterBase::StaticClass(), Found);

	ACharacterBase* Best = nullptr;
	int32 BestDist = TNumericLimits<int32>::Max();

	for (AActor* Actor : Found)
	{
		ACharacterBase* Target = Cast<ACharacterBase>(Actor);
		if (!Target) continue;
		if (Target == this) continue;

		if (Target->GetTeam() != 0) continue;

		const int32 Dist = ManhattanDistance(Target->CurrentTile, CurrentTile);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = Target;
		}
	}

	return Best;
}

int32 AEnemy::ManhattanDistance(const FTileCoord& A, const FTileCoord& B)
{
	return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

ACharacterBase* AEnemy::FindBestTarget(int32 _Range, int32 _BestRatio, int32 _Team)
{
	auto HealthRatio = [](const ACharacterBase* Unit) -> float
	{
		if (!Unit || !Unit->HealthComp)
		{
			return 1.0f;
		}
		Unit->HealthComp->GetMaxHealth();
		if (Unit->HealthComp->GetMaxHealth() <= 0.f)
		{
			return 1.0f;
		}
		return Unit->HealthComp->GetCurrentHealth() / Unit->HealthComp->GetMaxHealth();
	};

	ACharacterBase* BestTarget = nullptr;
	float BestRatio = _BestRatio; // se establece a 1 para que busque por debajo (curar) o cualquiera <= 1 mejora (atacar)
	int32 BestDist = TNumericLimits<int32>::Max();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacterBase::StaticClass(), Found);

	for (AActor* A : Found)
	{
		ACharacterBase* Char = Cast<ACharacterBase>(A);
		if (!Char)
		{
			continue;
		}

		// Solo un team
		if (Char->GetTeam() != _Team)
		{
			continue;
		}

		const int32 Dist = ManhattanDistance(CurrentTile, Char->CurrentTile);
		if (Dist > _Range)
		{
			continue;
		}

		const float Ratio = HealthRatio(Char);

		// Atacar/curar al que menos vida tenga. Empate -> más cercano.
		const bool bBetter =
			(Ratio < BestRatio) ||
			(FMath::IsNearlyEqual(Ratio, BestRatio) && Dist < BestDist);

		if (bBetter)
		{
			BestRatio = Ratio;
			BestDist = Dist;
			BestTarget = Char;
		}
	}

	return BestTarget;
}

const UActionDataAsset* AEnemy::GetActionById(EActionId ActionId) const
{
	if (!ArchetypeData)
	{
		return nullptr;
	}

	for (const UActionDataAsset* Action : ArchetypeData->Actions)
	{
		if (Action && Action->ActionId == ActionId)
		{
			return Action;
		}
	}
	return nullptr;
}
