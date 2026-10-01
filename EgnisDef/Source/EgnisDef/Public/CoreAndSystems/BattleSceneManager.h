#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BattleSceneManager.generated.h"

class UBattleManager;
class AGameManager;

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

	void Initialize(UBattleManager* InBattleManager);
	
	virtual UWorld* GetWorld() const override;
	
	// Estado actual
	UPROPERTY(BlueprintReadOnly, Category = "Scene Manager")
	EScenePhase CurrentPhase;

	// Referencia al Battle Manager
	UPROPERTY(BlueprintReadOnly, Category = "Scene Manager")
	UBattleManager* BattleManager;

	// --- FLUJO DEL JUEGO ---
    
	UFUNCTION(BlueprintCallable, Category = "Scene Director|Flow")
	void StartDeployment();

	UFUNCTION(BlueprintCallable, Category = "Scene Director|Flow")
	void StartCombatPhase();

	// Función que se ejecutará automáticamente cuando el combate termine
	UFUNCTION()
	void OnCombatEnded(bool bPlayerWon);

	// --- EVENTOS DE INTERFAZ ---
    
	UFUNCTION(BlueprintImplementableEvent, Category = "Scene Director|UI")
	void ShowDeploymentUI();

	UFUNCTION(BlueprintImplementableEvent, Category = "Scene Director|UI")
	void ShowRewardsUI(bool bPlayerWon);
};
