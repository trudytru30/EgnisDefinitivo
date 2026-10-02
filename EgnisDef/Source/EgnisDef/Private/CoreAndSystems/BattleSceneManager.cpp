#include "CoreAndSystems/BattleSceneManager.h"

#include "Cards/DeckManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/DeployManager.h"
#include "CoreAndSystems/GameManager.h"
#include "Kismet/GameplayStatics.h"

UBattleSceneManager::UBattleSceneManager()
{
	CurrentPhase = EScenePhase::Deployment;
}

void UBattleSceneManager::Initialize()
{
	// 1. Creamos el Gestor de Despliegue
	DeployManager = NewObject<UDeployManager>(this);
	check(DeployManager);

	// 2. Creamos el Gestor de Cartas
	DeckManager = NewObject<UDeckManager>(this);
	check(DeckManager);

	// 3. Creamos el Gestor de Batalla y le pasamos el mazo
	BattleManager = NewObject<UBattleManager>(this);
	check(BattleManager);
	BattleManager->SetDeckManager(DeckManager);

	// Nos suscribimos al final del combate
	BattleManager->OnBattleEndedEvent.AddDynamic(this, &UBattleSceneManager::OnCombatEnded);

	// Empezamos la escena
	StartDeploymentPhase();
}

UWorld* UBattleSceneManager::GetWorld() const
{
	// Si es el objeto por defecto de la clase (CDO), no tiene mundo
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	// Devuelve el mundo del 'Outer' (que es el GameManager/GameMode que lo creó con NewObject)
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void UBattleSceneManager::StartDeploymentPhase()
{
	CurrentPhase = EScenePhase::Deployment;
	UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: --- FASE DE DESPLIEGUE ---"));
	DeployManager->Initialize(BaseAllyClass);
	ShowDeploymentUI();
}

void UBattleSceneManager::StartCombatPhase()
{
	if (CurrentPhase != EScenePhase::Deployment) return;
	
	HideDeploymentUI();

	//Provisional de debug hasta que haya un sistema en run subsystems donde se guardan las cartas obtenidas
	DeckManager->SetDeck(InitialDeck);
	DeckManager->InitializeDeck();
	
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