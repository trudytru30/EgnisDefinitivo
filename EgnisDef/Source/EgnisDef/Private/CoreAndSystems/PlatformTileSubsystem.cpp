#include "CoreAndSystems/PlatformTileSubsystem.h"

#include "Characters/CharacterBase.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisPlatform, Log, All);

bool UPlatformTileSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UPlatformTileSubsystem::IsPlatformType(ETileType Type)
{
	return Type == ETileType::WaterPlatform || Type == ETileType::Ice;
}

ABoard* UPlatformTileSubsystem::FindBoard() const
{
	if (CachedBoard.IsValid()) return CachedBoard.Get();
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABoard> It(World); It; ++It)
		{
			CachedBoard = *It;
			return *It;
		}
	}
	return nullptr;
}

int32 UPlatformTileSubsystem::GetStandTurnsForLevel() const
{
	const ABoard* Board = FindBoard();
	return (Board && Board->CurrentLevelData) ? FMath::Max(1, Board->CurrentLevelData->PlatformStandTurns) : 3;
}

int32 UPlatformTileSubsystem::GetSunkTurnsForLevel() const
{
	const ABoard* Board = FindBoard();
	return (Board && Board->CurrentLevelData) ? FMath::Max(1, Board->CurrentLevelData->PlatformSunkTurns) : 4;
}

int32 UPlatformTileSubsystem::GetSunkTurnsLeft(const FTileCoord& Tile) const
{
	const int32* Found = SunkTurnsLeft.Find(Tile);
	return Found ? *Found : 0;
}

int32 UPlatformTileSubsystem::GetStandTurnsLeft(const FTileCoord& Tile) const
{
	const int32* Found = StandTurnsLeft.Find(Tile);
	return Found ? *Found : 0;
}

void UPlatformTileSubsystem::NotifyCharacterMoved(ACharacterBase* Character, const FTileCoord& LeftTile, const FTileCoord& NewTile)
{
	ABoard* Board = FindBoard();
	if (!Board || !IsValid(Character)) return;

	// Sale de una plataforma: se hunde en el momento
	if (Board->IsInside(LeftTile) && IsPlatformType(Board->GetTileType(LeftTile)) && !IsPlatformSunk(LeftTile))
	{
		StandTurnsLeft.Remove(LeftTile);
		Sink(LeftTile, TEXT("se queda vacia"));
	}

	// Pisa una plataforma: empieza a contar
	if (Board->IsInside(NewTile) && IsPlatformType(Board->GetTileType(NewTile)))
	{
		const int32 Turns = GetStandTurnsForLevel();
		StandTurnsLeft.Add(NewTile, Turns);
		UE_LOG(LogEgnisPlatform, Log, TEXT("[Platform] %s pisa la plataforma (%d,%d): aguanta %d turnos."),
			*Character->GetName(), NewTile.X, NewTile.Y, Turns);
	}
}

void UPlatformTileSubsystem::AdvancePlatforms()
{
	ABoard* Board = FindBoard();
	if (!Board) return;

	//as hundidas cuentan para volver a salir
	TArray<FTileCoord> ToRise;
	for (TPair<FTileCoord, int32>& Pair : SunkTurnsLeft)
	{
		if (--Pair.Value <= 0) ToRise.Add(Pair.Key);
	}
	for (const FTileCoord& Tile : ToRise)
	{
		Rise(Tile);
	}

	//Las pisadas gastan un turno. Las que llegan a 0 se hunden con quien este encima
	TArray<FTileCoord> ToSink;
	for (TPair<FTileCoord, int32>& Pair : StandTurnsLeft)
	{
		if (--Pair.Value <= 0) ToSink.Add(Pair.Key);
	}
	for (const FTileCoord& Tile : ToSink)
	{
		StandTurnsLeft.Remove(Tile);
		Sink(Tile, TEXT("no aguanta mas"));
	}

	//Alguien encima que no ha llegado andando (p. ej. colocado al desplegar): empieza a contar ahora
	const int32 Turns = GetStandTurnsForLevel();
	for (int32 X = 0; X < Board->BoardSizeX; ++X)
	{
		for (int32 Y = 0; Y < Board->BoardSizeY; ++Y)
		{
			const FTileCoord Tile(X, Y);
			if (!IsPlatformType(Board->GetTileType(Tile)) || IsPlatformSunk(Tile) || StandTurnsLeft.Contains(Tile)) continue;
			if (const ACharacterBase* Character = Cast<ACharacterBase>(Board->GetTileOccupant(Tile)))
			{
				StandTurnsLeft.Add(Tile, Turns);
				UE_LOG(LogEgnisPlatform, Log, TEXT("[Platform] %s ya estaba en la plataforma (%d,%d): aguanta %d turnos."),
					*Character->GetName(), X, Y, Turns);
			}
		}
	}
}

void UPlatformTileSubsystem::SinkNow(const FTileCoord& Tile)
{
	StandTurnsLeft.Remove(Tile);
	Sink(Tile, TEXT("comando"));
}

void UPlatformTileSubsystem::Sink(const FTileCoord& Tile, const TCHAR* Reason)
{
	ABoard* Board = FindBoard();
	if (!Board || !Board->IsInside(Tile) || IsPlatformSunk(Tile)) return;

	const int32 Turns = GetSunkTurnsForLevel();
	SunkTurnsLeft.Add(Tile, Turns);

	//Si hay alguien encima, cae al agua y muere
	if (ACharacterBase* Character = Cast<ACharacterBase>(Board->GetTileOccupant(Tile)))
	{
		UE_LOG(LogEgnisPlatform, Log, TEXT("[Platform] %s cae al agua en (%d,%d) y muere."),
			*Character->GetName(), Tile.X, Tile.Y);
		UEgnisCombatLibrary::ApplyPureDamage(Character, Character->GetCurrentHealth());
	}

	Board->SetTileBlocked(Tile, true);
	UE_LOG(LogEgnisPlatform, Log, TEXT("[Platform] (%d,%d) se HUNDE (%s). Vuelve en %d turnos."),
		Tile.X, Tile.Y, Reason, Turns);
	OnPlatformChanged.Broadcast(Tile, true);
}

void UPlatformTileSubsystem::Rise(const FTileCoord& Tile)
{
	SunkTurnsLeft.Remove(Tile);
	if (ABoard* Board = FindBoard())
	{
		Board->SetTileBlocked(Tile, false);
	}
	UE_LOG(LogEgnisPlatform, Log, TEXT("[Platform] (%d,%d) vuelve a SALIR."), Tile.X, Tile.Y);
	OnPlatformChanged.Broadcast(Tile, false);
}