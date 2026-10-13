#include "CoreAndSystems/GameManager.h"
#include "Characters/BoardPlayerController.h"
#include "CoreAndSystems/AudioDataAsset.h"
#include "CoreAndSystems/AudioManager.h"
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

	UClass* ClassToUse = BattleSceneManagerClass ? BattleSceneManagerClass.Get() : UBattleSceneManager::StaticClass();
    
	BattleSceneManager = NewObject<UBattleSceneManager>(this, ClassToUse);
	check(BattleSceneManager);
    
	BattleSceneManager->Initialize();
}