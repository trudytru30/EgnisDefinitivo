// Play -> clic en la ventana del juego -> tecla º -> comando -> enter y mirar registro
//
//   Egnis.Fire.List                      -> casillas en llamas con TurnsLeft y daño
//   Egnis.Fire.Add <X> <Y> [Turns] [Dmg] -> enciende una casilla (por defecto 1 turno, 2 de daño)
//   Egnis.Fire.Clear                     -> apaga todas
//   Egnis.Fire.Burn <X> <Y> [Turns]      -> aplica Quemadura al personaje de esa casilla
//   Egnis.Fire.Move <X> <Y> <DestX> <DestY> -> mueve al personaje de (X,Y) a destino por SetCurrentTile
//                                              (pasa por el mismo camino que jugador, enemigos y cartas)
//   Egnis.Fire.Turn                      -> muestra de quién es el turno

#include "CoreAndSystems/Board.h"
#include "Characters/CharacterBase.h"
#include "Components/StatusEffectComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	ABoard* FindBoard(UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<ABoard> It(World); It; ++It) return *It;
		UE_LOG(LogTemp, Warning, TEXT("[FireDebug] No hay Board (¿estas en Play?)."));
		return nullptr;
	}

	bool HasArgs(const TArray<FString>& Args, int32 Min, const TCHAR* Usage)
	{
		if (Args.Num() >= Min) return true;
		UE_LOG(LogTemp, Warning, TEXT("[FireDebug] Uso: %s"), Usage);
		return false;
	}

	ACharacterBase* CharacterAt(ABoard* Board, int32 X, int32 Y)
	{
		FTileCoord Tile; Tile.X = X; Tile.Y = Y;
		ACharacterBase* Character = Cast<ACharacterBase>(Board->GetTileOccupant(Tile));
		if (!Character)
		{
			UE_LOG(LogTemp, Warning, TEXT("[FireDebug] No hay ningun personaje en (%d,%d)."), X, Y);
		}
		return Character;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdFireList(
		TEXT("Egnis.Fire.List"), TEXT("Lista las casillas en llamas."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			ABoard* Board = FindBoard(World);
			if (!Board) return;
			const TArray<FFireTile> Tiles = Board->GetFireTiles();
			UE_LOG(LogTemp, Log, TEXT("[FireDebug] %d casilla(s) en llamas"), Tiles.Num());
			for (const FFireTile& F : Tiles)
			{
				UE_LOG(LogTemp, Log, TEXT("[FireDebug]   (%d,%d) TurnsLeft=%d daño=%.0f"),
					F.Tile.X, F.Tile.Y, F.TurnsLeft, F.Damage);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdFireAdd(
		TEXT("Egnis.Fire.Add"), TEXT("Uso: Egnis.Fire.Add <X> <Y> [Turns] [Dmg]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!HasArgs(Args, 2, TEXT("Egnis.Fire.Add <X> <Y> [Turns] [Dmg]"))) return;
			ABoard* Board = FindBoard(World);
			if (!Board) return;
			FTileCoord Tile; Tile.X = FCString::Atoi(*Args[0]); Tile.Y = FCString::Atoi(*Args[1]);
			const int32 Turns = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 1;
			const float Dmg = Args.Num() > 3 ? FCString::Atof(*Args[3]) : 2.f;
			Board->AddFireTile(Tile, Turns, Dmg);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdFireClear(
		TEXT("Egnis.Fire.Clear"), TEXT("Apaga todas las casillas en llamas."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			ABoard* Board = FindBoard(World);
			if (!Board) return;
			// Avanzar hasta vaciar: no necesito un Clear nuevo en Board. Máx. 100 vueltas por seguridad.
			for (int32 i = 0; i < 100 && Board->GetFireTiles().Num() > 0; ++i) Board->AdvanceFireTiles();
			UE_LOG(LogTemp, Log, TEXT("[FireDebug] Casillas en llamas apagadas."));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdFireBurn(
		TEXT("Egnis.Fire.Burn"), TEXT("Uso: Egnis.Fire.Burn <X> <Y> [Turns]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!HasArgs(Args, 2, TEXT("Egnis.Fire.Burn <X> <Y> [Turns]"))) return;
			ABoard* Board = FindBoard(World);
			if (!Board) return;
			ACharacterBase* C = CharacterAt(Board, FCString::Atoi(*Args[0]), FCString::Atoi(*Args[1]));
			if (!C || !C->StatusComp) return;
			const int32 Turns = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 3;
			C->StatusComp->ApplyStatus(EStatusType::Status_Burn, Turns);
			UE_LOG(LogTemp, Log, TEXT("[FireDebug] %s ahora tiene Quemadura %d turnos (HasStatus=%d)"),
				*C->GetName(), Turns, C->StatusComp->HasStatus(EStatusType::Status_Burn));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdFireMove(
		TEXT("Egnis.Fire.Move"), TEXT("Uso: Egnis.Fire.Move <X> <Y> <DestX> <DestY>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!HasArgs(Args, 4, TEXT("Egnis.Fire.Move <X> <Y> <DestX> <DestY>"))) return;
			ABoard* Board = FindBoard(World);
			if (!Board) return;
			ACharacterBase* C = CharacterAt(Board, FCString::Atoi(*Args[0]), FCString::Atoi(*Args[1]));
			if (!C) return;
			FTileCoord Dest; Dest.X = FCString::Atoi(*Args[2]); Dest.Y = FCString::Atoi(*Args[3]);
			const FString Name = C->GetName();
			const bool bOk = C->SetCurrentTile(Dest);
			UE_LOG(LogTemp, Log, TEXT("[FireDebug] SetCurrentTile(%s -> %d,%d) = %s"),
				*Name, Dest.X, Dest.Y, bOk ? TEXT("OK") : TEXT("FALLO (ocupada/fuera)"));
			if (bOk && IsValid(C)) C->SnapToCurrentTile(false); // sin esto solo se mueve la lógica, no el actor
		}));
}