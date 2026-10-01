#include "CoreAndSystems/GameManager.h"
#include "Characters/BoardPlayerController.h"
#include "CoreAndSystems/AudioDataAsset.h"
#include "CoreAndSystems/AudioManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "Cards/DeckManager.h"
#include "CoreAndSystems/BattleSceneManager.h"

AGameManager::AGameManager()
{
	PlayerControllerClass = ABoardPlayerController::StaticClass();
}

void AGameManager::BeginPlay()
{
	Super::BeginPlay();

	InitializeManagers();
}

void AGameManager::InitializeManagers()
{
	if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>())
		AM->SetAudioData(AudioData);

	// 1. Deck Manager
	DeckManager = NewObject<UDeckManager>(this);
	check(DeckManager);

	UE_LOG(LogTemp, Warning, TEXT("InitialDeck size: %d"), InitialDeck.Num());
	for (UBaseCard* Card : InitialDeck)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card: %s"), Card ? *Card->GetName() : TEXT("NULL"));
	}

	DeckManager->SetDeck(InitialDeck);

	UE_LOG(LogTemp, Warning, TEXT("DrawPile after init: %d"), DeckManager->GetDrawPileSize());

	// 2. Battle Manager
	BattleManager = NewObject<UBattleManager>(this);
	check(BattleManager);
	BattleManager->Initialize(DeckManager);	// Aqui se inicializa el mazo

	UE_LOG(LogTemp, Warning, TEXT("Hand after StartBattle: %d"), DeckManager->GetHand().Num());

	// 3. Battle Scene Manager (Arranca en Fase de Despliegue)
	UClass* ClassToUse = BattleSceneManagerClass ? BattleSceneManagerClass.Get() : UBattleSceneManager::StaticClass();
    
	BattleSceneManager = NewObject<UBattleSceneManager>(this, ClassToUse);
	check(BattleSceneManager);
    
	BattleSceneManager->Initialize(BattleManager);
}