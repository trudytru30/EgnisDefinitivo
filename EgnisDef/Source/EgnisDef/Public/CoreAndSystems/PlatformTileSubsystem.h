#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CoreAndSystems/Board.h"
#include "PlatformTileSubsystem.generated.h"

class ACharacterBase;

// Salta cuando una plataforma se hunde (bSunk = true) o vuelve a salir (false)
// Para los VFX
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlatformChanged, FTileCoord, Tile, bool, bSunk);

/**
 * Nenufares (WaterPlatform) y plataformas de escarcha (Ice), r1
 * - Al pisarla, aguanta PlatformStandTurns turnos con alguien encima
 * Al llegar a 0 se hunde
 * - Al salir de ella se hunde en el momento
 * - Si se hunde con alguien encima, cae al agua y muere
 * - Hundida = casilla bloqueada (Board->SetTileBlocked)
 * vuelve tras PlatformSunkTurns turnos.
 * BattleManager llama a AdvancePlatforms al empezar cada turno (de cualquier bando)
 */
UCLASS()
class EGNISDEF_API UPlatformTileSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintPure, Category="Combat|Platform")
	static bool IsPlatformType(ETileType Type);

	// Lo llama CharacterBase::SetCurrentTile
	void NotifyCharacterMoved(ACharacterBase* Character, const FTileCoord& LeftTile, const FTileCoord& NewTile);

	// Lo llama BattleManager al empezar cada turno
	void AdvancePlatforms();

	// Para pruebas (comando Egnis.Platform.Sink)
	void SinkNow(const FTileCoord& Tile);

	UFUNCTION(BlueprintPure, Category="Combat|Platform")
	bool IsPlatformSunk(const FTileCoord& Tile) const { return SunkTurnsLeft.Contains(Tile); }

	UFUNCTION(BlueprintPure, Category="Combat|Platform")
	int32 GetSunkTurnsLeft(const FTileCoord& Tile) const;

	UFUNCTION(BlueprintPure, Category="Combat|Platform")
	int32 GetStandTurnsLeft(const FTileCoord& Tile) const;

	UPROPERTY(BlueprintAssignable, Category="Combat|Platform")
	FOnPlatformChanged OnPlatformChanged;

private:
	void Sink(const FTileCoord& Tile, const TCHAR* Reason);
	void Rise(const FTileCoord& Tile);
	ABoard* FindBoard() const;
	int32 GetStandTurnsForLevel() const;
	int32 GetSunkTurnsForLevel() const;

	TMap<FTileCoord, int32> StandTurnsLeft;
	TMap<FTileCoord, int32> SunkTurnsLeft;
	mutable TWeakObjectPtr<ABoard> CachedBoard;
};