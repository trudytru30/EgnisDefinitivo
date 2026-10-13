#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameManager.generated.h"

class UBattleSceneManager;
class UAudioDataAsset;

UCLASS()
class EGNISDEF_API AGameManager : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGameManager();

	UFUNCTION(BlueprintPure, Category = "Managers")
	UBattleSceneManager* GetBattleSceneManager() const { return BattleSceneManager; }

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category="Audio")
	TObjectPtr<UAudioDataAsset> AudioData;

	UPROPERTY(EditDefaultsOnly, Category="Scene Director")
	TSubclassOf<UBattleSceneManager> BattleSceneManagerClass;

private:
	void InitializeManagers();

	UPROPERTY()
	TObjectPtr<UBattleSceneManager> BattleSceneManager = nullptr;
};