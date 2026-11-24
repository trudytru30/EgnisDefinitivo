#include "CoreAndSystems/GeyserSubsystem.h"

#include "Characters/CharacterBase.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "CoreAndSystems/EgnisEnvironmentSettings.h"
#include "CoreAndSystems/LevelDataAsset.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisGeyser, Log, All);

static TAutoConsoleVariable<int32> CVarGeyserDebug(
	TEXT("Egnis.Geyser.Debug"), 1,
	TEXT("1 = dibuja una caja sobre cada geiser (azul inactivo, amarillo aviso, rojo erupcion). 0 = no dibujar"),
	ECVF_Default);

bool UGeyserSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGeyserSubsystem::Deinitialize()
{
	Clear();
	Super::Deinitialize();
}

ABoard* UGeyserSubsystem::FindBoard() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	for (TActorIterator<ABoard> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void UGeyserSubsystem::Setup()
{
	Clear();
	bSetupPending = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() { DoSetup(); }));
	}
}

void UGeyserSubsystem::DoSetup()
{
	if (!bSetupPending) return; // se limpio antes de llegar
	bSetupPending = false;

	ABoard* Board = FindBoard();
	if (!Board)
	{
		UE_LOG(LogEgnisGeyser, Warning, TEXT("[Geiser] No hay Board: no se colocan geiseres."));
		return;
	}

	const UEgnisEnvironmentSettings* Settings = GetDefault<UEgnisEnvironmentSettings>();
	const ULevelDataAsset* Data = Board->CurrentLevelData;

	if (Data && Data->GeyserTiles.Num() > 0)
	{
		for (const FTileCoord& Tile : Data->GeyserTiles)
		{
			if (!Board->IsInside(Tile) || !ULevelDataAsset::IsDeployableTileType(Board->GetTileType(Tile)))
			{
				UE_LOG(LogEgnisGeyser, Warning, TEXT("[Geiser] (%d,%d) no es una casilla valida, se ignora."), Tile.X, Tile.Y);
				continue;
			}
			Tiles.AddUnique(Tile);
		}
	}
	else
	{
		// Sin casillas en el nivel: al azar entre las libres
		TArray<FTileCoord> Candidates;
		for (int32 Y = 0; Y < Board->BoardSizeY; ++Y)
		{
			for (int32 X = 0; X < Board->BoardSizeX; ++X)
			{
				const FTileCoord Tile(X, Y);
				if (ULevelDataAsset::IsDeployableTileType(Board->GetTileType(Tile)) && !Board->IsTileOccupied(Tile))
				{
					Candidates.Add(Tile);
				}
			}
		}
		const int32 Count = FMath::Min(Settings->GeyserDefaultCount, Candidates.Num());
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Pick = FMath::RandRange(i, Candidates.Num() - 1);
			Candidates.Swap(i, Pick);
			Tiles.Add(Candidates[i]);
		}
		UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] El nivel no define GeyserTiles: %d al azar."), Tiles.Num());
	}

	for (const FTileCoord& Tile : Tiles)
	{
		UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] Geiser en (%d,%d)"), Tile.X, Tile.Y);
	}

#if ENABLE_DRAW_DEBUG
	if (Tiles.Num() > 0 && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(DebugDrawTimer,
			FTimerDelegate::CreateWeakLambda(this, [this]() { DrawDebug(); }), 0.2f, true);
	}
#endif

	SetState(EGeyserState::Idle);
}

void UGeyserSubsystem::BeginCombat()
{
	// Por si el combate empieza antes de que se hayan colocado
	if (bSetupPending) DoSetup();

	bCombatRunning = true;
	Round = 0;
	ActiveLeft = 0;
	SetState(EGeyserState::Idle);
}

void UGeyserSubsystem::Clear()
{
	bSetupPending = false;
	bCombatRunning = false;
	Round = 0;
	ActiveLeft = 0;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DebugDrawTimer);
	}

	if (ABoard* Board = FindBoard())
	{
		for (const FTileCoord& Tile : Tiles)
		{
			Board->SetTileBlocked(Tile, false);
		}
	}
	const bool bHadTiles = Tiles.Num() > 0;
	Tiles.Reset();
	State = EGeyserState::Idle;
	if (bHadTiles)
	{
		OnGeyserStateChanged.Broadcast(EGeyserState::Idle);
	}
}

void UGeyserSubsystem::SetState(EGeyserState NewState)
{
	const bool bChanged = (State != NewState);
	State = NewState;
	if (bChanged || NewState == EGeyserState::Idle)
	{
		OnGeyserStateChanged.Broadcast(NewState);
	}
}

void UGeyserSubsystem::OnPlayerTurnStarted()
{
	if (!bCombatRunning || Tiles.Num() == 0) return;

	const UEgnisEnvironmentSettings* Settings = GetDefault<UEgnisEnvironmentSettings>();
	const int32 Period = FMath::Max(2, Settings->GeyserEruptionPeriod);
	const int32 ActiveRounds = FMath::Clamp(Settings->GeyserActiveRounds, 1, Period - 1);

	++Round;

	if (State == EGeyserState::Erupting)
	{
		if (--ActiveLeft <= 0)
		{
			Retract();
		}
		else
		{
			UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] Ronda %d: siguen activos (%d ronda(s) mas)."), Round, ActiveLeft);
			return;
		}
	}

	if (Round % Period == 0)
	{
		ActiveLeft = ActiveRounds;
		Erupt();
	}
	else if (Settings->bGeyserWarnBeforeEruption && (Round + 1) % Period == 0)
	{
		UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] Ronda %d: AVISO, erupcionan al empezar la siguiente."), Round);
		SetState(EGeyserState::Warning);
	}
	else
	{
		SetState(EGeyserState::Idle);
	}
}

void UGeyserSubsystem::Erupt()
{
	ABoard* Board = FindBoard();
	if (!Board) return;

	const UEgnisEnvironmentSettings* Settings = GetDefault<UEgnisEnvironmentSettings>();
	UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] Ronda %d: ERUPCION en %d casilla(s)."), Round, Tiles.Num());

	// 1) Daño a quien este encima (antes de bloquear). Si muere, el actor se destruye
	TArray<TWeakObjectPtr<ACharacterBase>> ToPush;
	for (const FTileCoord& Tile : Tiles)
	{
		ACharacterBase* Character = Cast<ACharacterBase>(Board->GetTileOccupant(Tile));
		if (!Character) continue;

		const FString Name = Character->GetName();
		const bool bIsAlly = (Character->GetTeam() == 0);
		if ((bIsAlly || Settings->bGeyserDamagesEnemies) && Settings->GeyserDamage > 0.f)
		{
			const float HealthBefore = Character->GetCurrentHealth();
			UEgnisCombatLibrary::ApplyPureDamage(Character, Settings->GeyserDamage);
			UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] %s estaba en (%d,%d): daño puro %.0f, vida %.0f -> %.0f%s"),
				*Name, Tile.X, Tile.Y, Settings->GeyserDamage, HealthBefore,
				IsValid(Character) ? Character->GetCurrentHealth() : 0.f,
				IsValid(Character) ? TEXT("") : TEXT(" (MUERTO)"));
		}
		if (IsValid(Character))
		{
			ToPush.Add(Character);
		}
	}

	// 2) Se bloquean todas antes de empujar, para que nadie acabe empujado a otro geiser
	for (const FTileCoord& Tile : Tiles)
	{
		Board->SetTileBlocked(Tile, true);
	}

	// 3) Los que seguian vivos se empujan a una casilla libre contigua
	for (const TWeakObjectPtr<ACharacterBase>& Weak : ToPush)
	{
		if (ACharacterBase* Character = Weak.Get())
		{
			PushOffTile(Character);
		}
	}

	SetState(EGeyserState::Erupting);
}

void UGeyserSubsystem::PushOffTile(ACharacterBase* Character)
{
	ABoard* Board = Character ? Character->Board.Get() : nullptr;
	if (!Board) return;

	const FTileCoord From = Character->CurrentTile;
	TArray<FTileCoord> Free;
	Board->GetNeighbors4_Free(From, Free); // ya excluye bloqueadas, ocupadas, Void y fuera del tablero
	if (Free.Num() == 0)
	{
		UE_LOG(LogEgnisGeyser, Warning, TEXT("[Geiser] %s en (%d,%d) no tiene casilla libre contigua: se queda encima."),
			*Character->GetName(), From.X, From.Y);
		return;
	}

	// Se prefiere una casilla que no este en llamas
	FTileCoord Dest = Free[0];
	for (const FTileCoord& Candidate : Free)
	{
		if (!Board->IsTileOnFire(Candidate))
		{
			Dest = Candidate;
			break;
		}
	}

	const FString Name = Character->GetName();
	// Mismo camino que el empujar de las cartas. Puede matarlo (casilla en llamas): comprobar IsValid despues
	if (!Character->SetCurrentTile(Dest))
	{
		UE_LOG(LogEgnisGeyser, Warning, TEXT("[Geiser] SetCurrentTile rechazo (%d,%d) para %s."), Dest.X, Dest.Y, *Name);
		return;
	}
	if (IsValid(Character))
	{
		Character->SnapToCurrentTile(true);
	}
	UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] %s empujado de (%d,%d) a (%d,%d)."), *Name, From.X, From.Y, Dest.X, Dest.Y);
}

void UGeyserSubsystem::Retract()
{
	if (ABoard* Board = FindBoard())
	{
		for (const FTileCoord& Tile : Tiles)
		{
			Board->SetTileBlocked(Tile, false);
		}
	}
	UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] Ronda %d: los geiseres se retiran."), Round);
	SetState(EGeyserState::Idle);
}

void UGeyserSubsystem::DrawDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (CVarGeyserDebug.GetValueOnGameThread() == 0) return;

	const UWorld* World = GetWorld();
	const ABoard* Board = FindBoard();
	if (!World || !Board) return;

	const FColor Color = (State == EGeyserState::Erupting) ? FColor::Red
		: (State == EGeyserState::Warning) ? FColor::Yellow : FColor::Cyan;
	const float Half = Board->TileSize * 0.5f;
	for (const FTileCoord& Tile : Tiles)
	{
		FVector Center = Board->TileToWorldCenter(Tile);
		Center.Z += 20.f;
		DrawDebugBox(World, Center, FVector(Half, Half, 20.f), Color, false, 0.25f, 0, 3.f);
	}
#endif
}

void UGeyserSubsystem::DumpToLog() const
{
	UE_LOG(LogEgnisGeyser, Log, TEXT("[Geiser] estado=%s, ronda=%d, activo_restante=%d, combate=%d, geiseres=%d"),
		*UEnum::GetValueAsString(State), Round, ActiveLeft, bCombatRunning, Tiles.Num());
	const ABoard* Board = FindBoard();
	for (const FTileCoord& Tile : Tiles)
	{
		UE_LOG(LogEgnisGeyser, Log, TEXT("  (%d,%d) bloqueada=%d"), Tile.X, Tile.Y, Board ? Board->IsTileBlocked(Tile) : false);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GEgnisGeyserDumpCmd(
	TEXT("Egnis.Geyser.Dump"),
	TEXT("Muestra estado, ronda y casillas de los geiseres"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		if (UGeyserSubsystem* Sub = World ? World->GetSubsystem<UGeyserSubsystem>() : nullptr)
		{
			Sub->DumpToLog();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GEgnisGeyserNextCmd(
	TEXT("Egnis.Geyser.Next"),
	TEXT("Avanza una ronda de geiseres sin jugar el turno (solo para probar)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		if (UGeyserSubsystem* Sub = World ? World->GetSubsystem<UGeyserSubsystem>() : nullptr)
		{
			Sub->OnPlayerTurnStarted();
		}
	}));