#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/GameManager.h"
#include "Kismet/GameplayStatics.h"

ABattleSceneManager::ABattleSceneManager()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentPhase = EScenePhase::Deployment;
}

void ABattleSceneManager::BeginPlay()
{
	Super::BeginPlay();

	AGameManager* GM = Cast<AGameManager>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GM)
	{
		BattleManager = GM->GetBattleManager();
	}

	BattleManager->OnBattleEndedEvent.AddDynamic(this, &ABattleSceneManager::OnCombatEnded);

	StartDeployment();
}

void ABattleSceneManager::StartDeployment()
{
	CurrentPhase = EScenePhase::Deployment;
	UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: --- FASE DE DESPLIEGUE ---"));

	ShowDeploymentUI();
}

void ABattleSceneManager::StartCombatPhase()
{
	if (CurrentPhase != EScenePhase::Deployment) return;

	CurrentPhase = EScenePhase::Combat;
	UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: --- FASE DE COMBATE ---"));

	if (BattleManager)
	{
		BattleManager->StartBattle(); 
	}
}

void ABattleSceneManager::OnCombatEnded(bool bPlayerWon)
{
	CurrentPhase = EScenePhase::Rewards;
	UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: --- FASE DE RECOMPENSAS --- Victoria: %s"));

	ShowRewardsUI(bPlayerWon);
}