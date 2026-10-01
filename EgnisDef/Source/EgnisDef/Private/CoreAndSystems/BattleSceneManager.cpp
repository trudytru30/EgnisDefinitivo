#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/GameManager.h"
#include "Kismet/GameplayStatics.h"

UBattleSceneManager::UBattleSceneManager()
{
	CurrentPhase = EScenePhase::Deployment;
}

UWorld* UBattleSceneManager::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void UBattleSceneManager::Initialize(UBattleManager* InBattleManager)
{
	BattleManager = InBattleManager;

	if (BattleManager)
	{
		BattleManager->OnBattleEndedEvent.AddDynamic(this, &UBattleSceneManager::OnCombatEnded);
	}

	StartDeployment();
}

void UBattleSceneManager::StartDeployment()
{
	CurrentPhase = EScenePhase::Deployment;
	UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: --- FASE DE DESPLIEGUE ---"));

	ShowDeploymentUI();
}

void UBattleSceneManager::StartCombatPhase()
{
	if (CurrentPhase != EScenePhase::Deployment) return;

	CurrentPhase = EScenePhase::Combat;
	UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: --- FASE DE COMBATE ---"));

	if (BattleManager)
	{
		BattleManager->StartBattle(); 
	}
}

void UBattleSceneManager::OnCombatEnded(bool bPlayerWon)
{
	CurrentPhase = EScenePhase::Rewards;
	UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: --- FASE DE RECOMPENSAS ---"));

	ShowRewardsUI(bPlayerWon);
}