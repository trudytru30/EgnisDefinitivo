#include "CoreAndSystems/GameManager.h"
#include "Characters/BoardPlayerController.h"
#include "CoreAndSystems/AudioDataAsset.h"
#include "CoreAndSystems/AudioManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "Cards/DeckManager.h"

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

	DeckManager = NewObject<UDeckManager>(this);
	check(DeckManager);

	UE_LOG(LogTemp, Warning, TEXT("InitialDeck size: %d"), InitialDeck.Num());
	for (UBaseCard* Card : InitialDeck)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card: %s"), Card ? *Card->GetName() : TEXT("NULL"));
	}

	DeckManager->SetDeck(InitialDeck);

	UE_LOG(LogTemp, Warning, TEXT("DrawPile after init: %d"), DeckManager->GetDrawPileSize());

	BattleManager = NewObject<UBattleManager>(this);
	check(BattleManager);
	BattleManager->Initialize(DeckManager);	// Aqui se inicializa el mazo
	BattleManager->StartBattle();

	UE_LOG(LogTemp, Warning, TEXT("Hand after StartBattle: %d"), DeckManager->GetHand().Num());
}