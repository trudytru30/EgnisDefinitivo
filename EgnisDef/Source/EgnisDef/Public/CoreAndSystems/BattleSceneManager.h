#pragma once

#include "CoreMinimal.h"
#include "Characters/HeroDataAsset.h"
#include "BattleSceneManager.generated.h"

class UDeployManager;
class UBattleManager;
class UDeckManager;
class UBaseCard;

UENUM(BlueprintType)
enum class EScenePhase : uint8
{
	Deployment UMETA(DisplayName = "Fase de Despliegue"),
	Combat     UMETA(DisplayName = "Fase de Combate"),
	Rewards    UMETA(DisplayName = "Fase de Recompensas")
};

UCLASS(Blueprintable, BlueprintType)
class EGNISDEF_API UBattleSceneManager : public UObject
{
	GENERATED_BODY()
	
public:	
	UBattleSceneManager();

	virtual UWorld* GetWorld() const override;

	UPROPERTY(EditDefaultsOnly)
	TArray<UBaseCard*> InitialDeck;

	UPROPERTY(BlueprintReadOnly, Category = "Scene Manager")
	EScenePhase CurrentPhase;

	UFUNCTION(BlueprintPure, Category = "Managers")
	UBattleManager* GetBattleManager() const { return BattleManager; }

	UFUNCTION(BlueprintPure, Category = "Managers")
	UDeckManager* GetDeckManager() const { return DeckManager; }

	UFUNCTION(BlueprintPure, Category = "Managers")
	UDeployManager* GetDeployManager() const { return DeployManager; }

	void Initialize();

	// --- FLUJO DEL JUEGO ---
    
	UFUNCTION(BlueprintCallable, Category = "Scene Director|Flow")
	void StartDeploymentPhase();

	UFUNCTION(BlueprintCallable, Category = "Scene Director|Flow")
	void StartCombatPhase();

	//Condiciones para empezar el combate (de momento: estar en despliegue con al menos un héroe en el tablero)
	//La UI la usa para activar/desactivar el botón de Start Combat
	UFUNCTION(BlueprintPure, Category = "Scene Director|Flow")
	bool CanStartCombat() const;

	UFUNCTION()
	void OnCombatEnded(bool bPlayerWon);

	// --- EVENTOS DE INTERFAZ ---
    
	UFUNCTION(BlueprintImplementableEvent, Category = "Scene Director|UI")
	void ShowDeploymentUI();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene Director|UI")
	void HideDeploymentUI();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene Director|UI")
	void ShowHUD();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene Director|UI")
	void HideHUD();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene Director|UI")
	void ShowRewardsUI(bool bPlayerWon);

protected:
	// Esta es la variable que configuras en tu Blueprint del BattleSceneManager en el Editor:
	UPROPERTY(EditDefaultsOnly, Category = "Deployment")
	TSubclassOf<AAlly> BaseAllyClass;

	//Máximo de héroes que se pueden tener a la vez en el tablero durante el despliegue
	UPROPERTY(EditDefaultsOnly, Category = "Deployment", meta = (ClampMin = "1"))
	int32 MaxDeployedHeroes = 4;

	UPROPERTY(BlueprintReadOnly, Category = "Scene Manager")
	UDeployManager* DeployManager;

	UPROPERTY(BlueprintReadOnly, Category = "Scene Manager")
	UBattleManager* BattleManager;

	UPROPERTY(BlueprintReadOnly, Category = "Scene Manager")
	UDeckManager* DeckManager;
};
