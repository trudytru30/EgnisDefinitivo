#include "Characters/EnemyTurnDirector.h"
#include "Characters/Enemy.h"
#include "Characters/EnemyAIController.h"
#include "CoreAndSystems/BattleManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Algo/StableSort.h"

void UEnemyTurnDirector::Initialize(UBattleManager* InBattleManager, const FEnemyTurnSettings& InSettings)
{
	BattleManager = InBattleManager;
	Settings = InSettings;
}

void UEnemyTurnDirector::StartEnemyPhase(const TArray<AEnemy*>& Enemies, FOnEnemyPhaseFinished OnFinished)
{
	Abort();

	Queue.Reset();
	for (AEnemy* Enemy : Enemies)
	{
		if (IsValid(Enemy))
		{
			Queue.Add(Enemy);
		}
	}

	//Orden configurable: TurnOrder mas bajo primero; si empatan, se respeta el orden de aparicion
	Algo::StableSortBy(Queue, [](const TWeakObjectPtr<AEnemy>& E) { return E.IsValid() ? E->TurnOrder : MAX_int32; });

	FString OrderText;
	for (const TWeakObjectPtr<AEnemy>& E : Queue)
	{
		OrderText += FString::Printf(TEXT("%s(%d) "), *GetNameSafe(E.Get()), E.IsValid() ? E->TurnOrder : 0);
	}
	UE_LOG(LogTemp, Log, TEXT("[EnemyTurn] Orden: %s"), *OrderText);

	CurrentIndex = 0;
	OnPhaseFinished = OnFinished;
	bRunning = true;

	UE_LOG(LogTemp, Log, TEXT("[EnemyTurn] Empieza la fase enemiga: %d enemigos."), Queue.Num());
	Schedule(Settings.DelayBeforeFirstEnemy);
}

void UEnemyTurnDirector::Abort()
{
	StopListening();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StepTimer);
		World->GetTimerManager().ClearTimer(TimeoutTimer);
	}
	bRunning = false;
}

void UEnemyTurnDirector::Schedule(float Delay)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	//un temporizador de 0 segundos no se dispara en el siguiente frame
	if (Delay <= 0.f)
	{
		StepTimer = World->GetTimerManager().SetTimerForNextTick(this, &UEnemyTurnDirector::ProcessNextEnemy);
	}
	else
	{
		World->GetTimerManager().SetTimer(StepTimer, this, &UEnemyTurnDirector::ProcessNextEnemy, Delay, false);
	}
}

void UEnemyTurnDirector::ProcessNextEnemy()
{
	if (!bRunning)
	{
		return;
	}

	if (BattleManager && BattleManager->IsBattleOver())
	{
		Abort();
		return;
	}

	//saltar los enemigos que han muerto)
	while (Queue.IsValidIndex(CurrentIndex) && !Queue[CurrentIndex].IsValid())
	{
		++CurrentIndex;
	}

	//comprueba si se ha acabado la fase enemiga
	if (!Queue.IsValidIndex(CurrentIndex))
	{
		bRunning = false;
		UE_LOG(LogTemp, Log, TEXT("[EnemyTurn] Termina la fase enemiga."));
		OnPhaseFinished.ExecuteIfBound();
		return;
	}

	AEnemy* Enemy = Queue[CurrentIndex].Get();
	AEnemyAIController* Controller = Cast<AEnemyAIController>(Enemy->GetController());

	
	if (Controller && Controller->HasBrain())
	{
		CurrentController = Controller;
		Controller->OnTurnFinished.AddUniqueDynamic(this, &UEnemyTurnDirector::HandleEnemyTurnFinished);

		
		GetWorld()->GetTimerManager().SetTimer(TimeoutTimer, this, &UEnemyTurnDirector::HandleEnemyTimeout,
			Settings.MaxSecondsPerEnemy, false);

		if (Controller->StartTurn())
		{
			return;
		}

		
		GetWorld()->GetTimerManager().ClearTimer(TimeoutTimer);
		StopListening();
	}

	//enemigo sin bt
	UE_LOG(LogTemp, Log, TEXT("[EnemyTurn] %s actua con el comportamiento antiguo."), *Enemy->GetName());
	Enemy->MoveTowardClosesPlayer();
	if (IsValid(Enemy))
	{
		Enemy->MakeAction();
	}
	OnEnemyDone();
}

void UEnemyTurnDirector::HandleEnemyTurnFinished(AEnemyAIController* EnemyController)
{
	if (EnemyController != CurrentController.Get())
	{
		return;
	}

	StopListening();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimeoutTimer);
	}
	OnEnemyDone();
}

void UEnemyTurnDirector::HandleEnemyTimeout()
{
	AEnemyAIController* Controller = CurrentController.Get();
	UE_LOG(LogTemp, Warning, TEXT("[EnemyTurn] %s no ha terminado su turno en %.0f s: se le da por terminado."),
		Controller ? *GetNameSafe(Controller->GetPawn()) : TEXT("(enemigo muerto)"), Settings.MaxSecondsPerEnemy);

	//primero escucha
	StopListening();
	if (Controller)
	{
		Controller->FinishTurn();
	}
	OnEnemyDone();
}

void UEnemyTurnDirector::StopListening()
{
	if (AEnemyAIController* Controller = CurrentController.Get())
	{
		Controller->OnTurnFinished.RemoveDynamic(this, &UEnemyTurnDirector::HandleEnemyTurnFinished);
	}
	CurrentController.Reset();
}

void UEnemyTurnDirector::OnEnemyDone()
{
	// Comprobar si alguien ha muerto (y si se ha acabado el combate)
	if (BattleManager)
	{
		BattleManager->UpdateUnitsAlive();
		if (BattleManager->IsBattleOver())
		{
			Abort();
			return;
		}
	}

	++CurrentIndex;
	Schedule(Settings.DelayBetweenEnemies);
}