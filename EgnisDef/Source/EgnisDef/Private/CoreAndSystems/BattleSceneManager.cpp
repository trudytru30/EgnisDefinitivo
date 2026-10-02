#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "Characters/Ally.h"
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

bool UBattleSceneManager::TryDeployHeroAtLocation(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (CurrentPhase != EScenePhase::Deployment || !SelectedHero || !BaseAllyClass)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World) return false;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 1. Hacemos Spawn de la clase BASE genérica
	AAlly* SpawnedAlly = World->SpawnActor<AAlly>(BaseAllyClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (SpawnedAlly)
	{
		// 2. Le inyectamos los datos para que "se convierta" en el héroe correcto
		SpawnedAlly->InitializeFromData(SelectedHero);
        
		// 3. Limpiamos la selección
		SelectedHero = nullptr; 
        
		return true;
	}
	return false;
}

void UBattleSceneManager::StartCombatPhase()
{
	if (CurrentPhase != EScenePhase::Deployment) return;

	HideDeploymentUI();
	ShowHUD();

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

	HideHUD();
	ShowRewardsUI(bPlayerWon);
}