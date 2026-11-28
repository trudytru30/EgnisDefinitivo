// Pruebass
// Play -> clic en la ventana del juego -> tecla º -> comando -> enter y mirar el Output Log
//
//   Egnis.Tile <X> <Y>                -> que hay en la casilla (tipo, quien esta encima, plataforma, bloqueo)
//   Egnis.Obstacle.Hit <X> <Y> [Daño] -> daña al obstaculo de esa casilla (por defecto 999)
//   Egnis.Platform.Advance            -> gasta un turno a las plataformas (como al empezar un turno)
//   Egnis.Platform.Sink <X> <Y>       -> hunde ya la plataforma de esa casilla
//   Para mover personajes: Egnis.Fire.Move <X> <Y> <DestX> <DestY>

#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/PlatformTileSubsystem.h"
#include "Environment/ObstacleBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	// Nombres distintos a los de FireDebugCommands para que no choquen al compilar
	ABoard* ObstacleDbg_FindBoard(UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<ABoard> It(World); It; ++It) return *It;
		UE_LOG(LogTemp, Warning, TEXT("[ObstacleDebug] No hay Board (¿estas en Play?)."));
		return nullptr;
	}

	bool ObstacleDbg_ReadTile(const TArray<FString>& Args, FTileCoord& OutTile, const TCHAR* Usage)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ObstacleDebug] Uso: %s"), Usage);
			return false;
		}
		OutTile = FTileCoord(FCString::Atoi(*Args[0]), FCString::Atoi(*Args[1]));
		return true;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdObstacleDbgTile(
		TEXT("Egnis.Tile"), TEXT("Egnis.Tile X Y: que hay en la casilla."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ABoard* Board = ObstacleDbg_FindBoard(World);
			FTileCoord Tile;
			if (!Board || !ObstacleDbg_ReadTile(Args, Tile, TEXT("Egnis.Tile X Y"))) return;
			if (!Board->IsInside(Tile))
			{
				UE_LOG(LogTemp, Warning, TEXT("[ObstacleDebug] (%d,%d) esta fuera del tablero."), Tile.X, Tile.Y);
				return;
			}

			const ETileType Type = Board->GetTileType(Tile);
			const AActor* Occupant = Board->GetTileOccupant(Tile);
			UE_LOG(LogTemp, Log, TEXT("[ObstacleDebug] (%d,%d) tipo=%s encima=%s ocupada=%s bloqueada=%s llamas=%s"),
				Tile.X, Tile.Y, *UEnum::GetValueAsString(Type),
				Occupant ? *Occupant->GetName() : TEXT("nadie"),
				Board->IsTileOccupied(Tile) ? TEXT("si") : TEXT("no"),
				Board->IsTileBlocked(Tile) ? TEXT("si") : TEXT("no"),
				Board->IsTileOnFire(Tile) ? TEXT("si") : TEXT("no"));

			if (const AObstacleBase* Obstacle = Cast<AObstacleBase>(Occupant))
			{
				UE_LOG(LogTemp, Log, TEXT("[ObstacleDebug]   obstaculo: vida %.0f/%.0f destructible=%s"),
					Obstacle->GetCurrentHealth(), Obstacle->GetMaxHealth(), Obstacle->bDestructible ? TEXT("si") : TEXT("no"));
			}

			const UPlatformTileSubsystem* Platforms = World->GetSubsystem<UPlatformTileSubsystem>();
			if (Platforms && UPlatformTileSubsystem::IsPlatformType(Type))
			{
				UE_LOG(LogTemp, Log, TEXT("[ObstacleDebug]   plataforma: hundida=%s (vuelve en %d) turnos pisada que le quedan=%d"),
					Platforms->IsPlatformSunk(Tile) ? TEXT("si") : TEXT("no"),
					Platforms->GetSunkTurnsLeft(Tile), Platforms->GetStandTurnsLeft(Tile));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdObstacleDbgHit(
		TEXT("Egnis.Obstacle.Hit"), TEXT("Egnis.Obstacle.Hit X Y [Daño]: daña al obstaculo de la casilla."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ABoard* Board = ObstacleDbg_FindBoard(World);
			FTileCoord Tile;
			if (!Board || !ObstacleDbg_ReadTile(Args, Tile, TEXT("Egnis.Obstacle.Hit X Y [Daño]"))) return;

			AObstacleBase* Obstacle = Cast<AObstacleBase>(Board->GetTileOccupant(Tile));
			if (!Obstacle)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ObstacleDebug] No hay ningun obstaculo en (%d,%d)."), Tile.X, Tile.Y);
				return;
			}
			if (!Obstacle->bDestructible)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ObstacleDebug] %s no es destructible (bDestructible = false)."), *Obstacle->GetName());
				return;
			}

			const float Damage = Args.Num() >= 3 ? FCString::Atof(*Args[2]) : 999.f;
			UE_LOG(LogTemp, Log, TEXT("[ObstacleDebug] Golpe de %.0f a %s en (%d,%d)."), Damage, *Obstacle->GetName(), Tile.X, Tile.Y);
			Obstacle->ApplyDamage(Damage);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdObstacleDbgAdvance(
		TEXT("Egnis.Platform.Advance"), TEXT("Gasta un turno a las plataformas."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UPlatformTileSubsystem* Platforms = World ? World->GetSubsystem<UPlatformTileSubsystem>() : nullptr)
			{
				UE_LOG(LogTemp, Log, TEXT("[ObstacleDebug] Avanzo un turno las plataformas."));
				Platforms->AdvancePlatforms();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdObstacleDbgSink(
		TEXT("Egnis.Platform.Sink"), TEXT("Egnis.Platform.Sink X Y: hunde ya la plataforma."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ABoard* Board = ObstacleDbg_FindBoard(World);
			FTileCoord Tile;
			if (!Board || !ObstacleDbg_ReadTile(Args, Tile, TEXT("Egnis.Platform.Sink X Y"))) return;
			if (!UPlatformTileSubsystem::IsPlatformType(Board->GetTileType(Tile)))
			{
				UE_LOG(LogTemp, Warning, TEXT("[ObstacleDebug] (%d,%d) no es WaterPlatform ni Ice."), Tile.X, Tile.Y);
				return;
			}
			World->GetSubsystem<UPlatformTileSubsystem>()->SinkNow(Tile);
		}));
}