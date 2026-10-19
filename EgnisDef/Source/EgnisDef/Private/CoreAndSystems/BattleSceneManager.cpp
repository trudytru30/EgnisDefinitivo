#include "CoreAndSystems/BattleSceneManager.h"

#include "Cards/DeckManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/DeployManager.h"
#include "CoreAndSystems/GameManager.h"
#include "Kismet/GameplayStatics.h"
#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "Engine/GameInstance.h"                   

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
	DeployManager->Initialize(BaseAllyClass, MaxDeployedHeroes);
	ShowDeploymentUI();
}

bool UBattleSceneManager::CanStartCombat() const
{
	return CurrentPhase == EScenePhase::Deployment && DeployManager && DeployManager->HasDeployedHeroes();
}

void UBattleSceneManager::StartCombatPhase()
{
	if (CurrentPhase != EScenePhase::Deployment) return;

	if (!CanStartCombat())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleSceneManager]: No se puede empezar el combate: no hay ningún héroe desplegado"));
		return;
	}

	HideDeploymentUI();

	//Si hay una run activa, el mazo sale de la mochila de la run (RunProgressionSubsystem)
	//Si no (como al abrir este mapa directamente para probar)se usa InitialDeck como antes
	TArray<UBaseCard*> DeckToUse;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	URunProgressionSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunProgressionSubsystem>() : nullptr;

	if (Run && Run->IsRunActive() && Run->GetDeck().Num() > 0)
	{
		DeckToUse = Run->GetDeckCards();
		UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: Mazo de la run (%d cartas)"), DeckToUse.Num());
	}
	else
	{
		if (InitialDeck.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[BattleSceneManager]: ¡InitialDeck está vacío! Asígnale cartas en el BP_BattleSceneManager"));
		}

		//Provisional de debug hasta que haya un sistema en run subsystems donde se guardan las cartas obtenidas
		//se mantiene como backup cuando no hay run activa
		DeckToUse = InitialDeck;
		UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: Sin run activa: se usa InitialDeck (%d cartas)"), DeckToUse.Num());
	}

	DeckManager->SetDeck(DeckToUse);
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
	//para proteger: si el final del combate llega dos veces, la segunda se ignora
	//(evita heridas dobles o restar la fogata dos veces)
	if (CurrentPhase == EScenePhase::Rewards) return;

	CurrentPhase = EScenePhase::Rewards;
	UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: --- FASE DE RECOMPENSAS ---"));

	//guarda en la mochila de la run lo que ha pasado en el combate (solo si hay run activa)
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	URunProgressionSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunProgressionSubsystem>() : nullptr;

	if (Run && Run->IsRunActive() && DeployManager)
	{
		//Héroes muertos: si su personaje ya no existe o no le queda vida, recibe una herida
		for (const TPair<TWeakObjectPtr<AAlly>, FName>& Deployed : DeployManager->GetDeployedHeroes())
		{
			const AAlly* Ally = Deployed.Key.Get();
			if (!Ally || Ally->GetCurrentHealth() <= 0.f)
			{
				UE_LOG(LogTemp, Log, TEXT("[BattleSceneManager]: %s ha caido en combate."), *Deployed.Value.ToString());
				Run->AddRandomWound(Deployed.Value);
			}
		}

		//combate perdido: la fogata pierde una llama (si llega a 0, la run termina sola)
		if (!bPlayerWon)
		{
			Run->LoseCampfire();
		}
	}

	HideHUD();
	ShowRewardsUI(bPlayerWon);
}