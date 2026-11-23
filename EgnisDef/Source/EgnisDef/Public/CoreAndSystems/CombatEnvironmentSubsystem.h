#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatEnvironmentSubsystem.generated.h"

UENUM(BlueprintType)
enum class ECombatEnvironment : uint8
{
	None,
	Geiser,
	Fog,
	EvilRain,
	Other // un nombre que no conocemos (aviso en el log); el BP puede reaccionar igualmente
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatEnvironmentStarted, ECombatEnvironment, Environment, FName, EnvironmentName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatEnvironmentEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFogChanged, bool, bFogActive);

/**
 * Entorno del combate actual (el que dejo preparado un evento con la accion CombatEnv).
 * Lo avisa BattleSceneManager al empezar el despliegue, al empezar el combate y al acabarlo.
 */
UCLASS()
class EGNISDEF_API UCombatEnvironmentSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// --- Los llama BattleSceneManager ---
	void BeginDeployment();
	void BeginCombat();
	void EndCombat();

	// --- Debug (comandos Egnis.Env.*) ---
	bool DebugSetEnvironment(FName Name);
	void DebugClear();
	void DumpToLog() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Environment")
	ECombatEnvironment GetActiveEnvironment() const { return Active; }

	UFUNCTION(BlueprintPure, Category = "Combat|Environment")
	FName GetActiveEnvironmentName() const { return ActiveName; }

	UFUNCTION(BlueprintPure, Category = "Combat|Environment")
	bool IsFogActive() const { return bFogActive; }

	// Para luz roja, niebla visual, etc. (si un BP nace despues del despliegue, que lea GetActiveEnvironment al empezar)
	UPROPERTY(BlueprintAssignable, Category = "Combat|Environment")
	FOnCombatEnvironmentStarted OnEnvironmentStarted;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Environment")
	FOnCombatEnvironmentEnded OnEnvironmentEnded;

	// Salta al aparecer y al desaparecer la niebla (la UI puede ocultar paneles de enemigos)
	UPROPERTY(BlueprintAssignable, Category = "Combat|Environment")
	FOnFogChanged OnFogChanged;

	static ECombatEnvironment ParseEnvironment(FName Name);

private:
	void StartEnvironment(FName Name);
	void StopEnvironment();
	void SetFogActive(bool bActive);
	void SetEnemiesHiddenByFog(bool bHide);
	void ApplyEvilRain();

	ECombatEnvironment Active = ECombatEnvironment::None;
	FName ActiveName;
	bool bFogActive = false;
	bool bInDeployment = false;
	bool bEvilRainApplied = false;
};