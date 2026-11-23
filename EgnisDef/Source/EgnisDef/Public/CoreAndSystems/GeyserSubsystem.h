#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CoreAndSystems/Board.h"
#include "GeyserSubsystem.generated.h"

class ACharacterBase;

UENUM(BlueprintType)
enum class EGeyserState : uint8
{
	Idle,      // inactivo, se puede pisar
	Warning,   // erupciona al empezar la proxima ronda (solo aviso)
	Erupting   // activo: bloqueado, ya ha dañado y empujado
};

// Salta al colocarse los geiseres (Idle), al avisar, al erupcionar y al retirarse. Todos los geiseres comparten estado
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGeyserStateChanged, EGeyserState, NewState);

/**
 * Geiseres del entorno "Geiser". Lo controla UCombatEnvironmentSubsystem (colocar/limpiar/empezar)
 * y BattleManager (OnPlayerTurnStarted al empezar cada ronda).
 */
UCLASS()
class EGNISDEF_API UGeyserSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	// Elige las casillas (el siguiente frame, cuando ya existen enemigos y obstaculos)
	void Setup();
	// Empieza el combate: la ronda 0 y desde aqui los geiseres actuan
	void BeginCombat();
	// Quita geiseres y desbloquea casillas
	void Clear();
	// Lo llama BattleManager::StartPlayerTurn
	void OnPlayerTurnStarted();

	void DumpToLog() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Geyser")
	TArray<FTileCoord> GetGeyserTiles() const { return Tiles; }

	UFUNCTION(BlueprintPure, Category = "Combat|Geyser")
	EGeyserState GetGeyserState() const { return State; }

	UPROPERTY(BlueprintAssignable, Category = "Combat|Geyser")
	FOnGeyserStateChanged OnGeyserStateChanged;

private:
	void DoSetup();
	void SetState(EGeyserState NewState);
	void Erupt();
	void Retract();
	void PushOffTile(ACharacterBase* Character);
	ABoard* FindBoard() const;
	void DrawDebug() const;

	TArray<FTileCoord> Tiles;
	EGeyserState State = EGeyserState::Idle;
	bool bSetupPending = false;
	bool bCombatRunning = false;
	int32 Round = 0;
	int32 ActiveLeft = 0;
	FTimerHandle DebugDrawTimer;
};