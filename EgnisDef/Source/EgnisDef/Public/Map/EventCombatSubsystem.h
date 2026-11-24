#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventCombatSubsystem.generated.h"

/**
 * Guarda lo que un evento de mapa deja preparado para el combate que viene al acabar el dialogo:
 * que hay combate, que entorno tiene y que acciones se ejecutan al ganar o al perder.
 * Vive en el GameInstance, asi que sobrevive al cambio de nivel dialogo -> combate.
 */
UCLASS()
class EGNISDEF_API UEventCombatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- Lo llaman las acciones del Sheet (StartCombat, OnWin, OnLose, CombatEnv) ---
	void RequestCombat(FName LevelKey);
	void AddWinAction(const FString& Action);
	void AddLoseAction(const FString& Action);
	void SetEnvironment(FName Environment);

	/** Fin del dialogo: si hay combate pedido, lo pasa a "en curso" para este nodo y devuelve su clave de nivel. Si no hay, descarta lo preparado. */
	bool ConsumeCombatRequest(FName NodeID, FName& OutLevelKey);

	/** Fin del combate (lo llama BattleSceneManager): ejecuta las acciones de ganar o perder y limpia. */
	void OnCombatFinished(UObject* WorldContext, bool bPlayerWon);

	/** Entorno del combate actual ("Geiser", "Fog", "EvilRain"...). None si no hay. Para que el combate lo lea. */
	UFUNCTION(BlueprintPure, Category = "Map|Events")
	FName GetEnvironment() const { return Environment; }

	/** True si el combate que se juega ahora viene de un evento. */
	UFUNCTION(BlueprintPure, Category = "Map|Events")
	bool IsEventCombat() const { return bInCombat; }

	UFUNCTION(BlueprintCallable, Category = "Map|Events")
	void ClearAll();

	void DumpToLog() const;

private:
	UFUNCTION()
	void HandleRunChanged();

	bool bRequested = false;
	bool bInCombat = false;
	FName LevelKey;
	FName Environment;
	FName CombatNodeID;
	TArray<FString> WinActions;
	TArray<FString> LoseActions;
};