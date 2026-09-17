#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameManager.generated.h"

class UAudioDataAsset;
class UAudioManager;
class UBaseCard;
class UBattleManager;
class UDeckManager;


UCLASS()
class EGNISDEF_API AGameManager : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGameManager();

#pragma region Functions
	UDeckManager* GetDeckManager() const { return DeckManager; }
	UBattleManager* GetBattleManager() const { return BattleManager; }
#pragma endregion

protected:

	virtual void BeginPlay() override;
	UPROPERTY(EditDefaultsOnly, Category="Audio")
	TObjectPtr<UAudioDataAsset> AudioData;

	UPROPERTY(EditDefaultsOnly)
	TArray<UBaseCard*> InitialDeck;

private:

	UPROPERTY()
	UDeckManager* DeckManager;

	UPROPERTY()
	UBattleManager* BattleManager;

	void InitializeManagers();
};