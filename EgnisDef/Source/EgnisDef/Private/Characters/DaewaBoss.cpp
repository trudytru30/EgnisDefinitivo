#include "Characters/DaewaBoss.h"
#include "Characters/EnemyCombatLibrary.h"
#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/GameManager.h"
#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/HealthAttributeSet.h"
#include "CoreAndSystems/AudioManager.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

namespace
{
	//Casilla imposible: asi ninguna carta ni area alcanza a Daewa (no se le puede atacar)
	const FTileCoord DaewaOffBoardTile(-1000, -1000);

	UBattleManager* GetDaewaBattleManager(const UWorld* World)
	{
		if (!World) return nullptr;
		const AGameManager* GM = Cast<AGameManager>(World->GetAuthGameMode());
		const UBattleSceneManager* BSM = GM ? GM->GetBattleSceneManager() : nullptr;
		return BSM ? BSM->GetBattleManager() : nullptr;
	}

	bool IsDaewaLiveHero(ACharacterBase* Character)
	{
		return IsValid(Character) && Character->GetTeam() == 0 && Character->GetCurrentHealth() > 0.f;
	}

	//GE "Add" con Set By Caller (daño negativo)
	bool ApplyDaewaDamage(ACharacterBase* Target, TSubclassOf<UGameplayEffect> Effect, float Damage, AActor* Instigator)
	{
		UAbilitySystemComponent* ASC = Target ? Target->GetAbilitySystemComponent() : nullptr;
		if (!ASC || !Effect) return false;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(Instigator, Instigator);
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1.f, Context);
		if (!Spec.IsValid()) return false;

		Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -Damage);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		return true;
	}

	const EDaewaSide DaewaSideOrder[4] = { EDaewaSide::XMax, EDaewaSide::YMax, EDaewaSide::XMin, EDaewaSide::YMin };
}

ADaewaBoss::ADaewaBoss()
{
	//AJUSTABLE: orden en el turno enemigo. Negativo = actua antes que el resto
	TurnOrder = -100;
}

void ADaewaBoss::BeginPlay()
{
	Super::BeginPlay();

	BaseZ = GetActorLocation().Z;

	//Sale del tablero: libera la casilla donde lo puso el nivel
	if (Board)
	{
		Board->UnregisterOccupant(CurrentTile, this);
	}
	CurrentTile = DaewaOffBoardTile;

	//Vida al aparecer (tambien es la maxima, asi la barra muestra la bajada)
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->SetNumericAttributeBase(UHealthAttributeSet::GetMaxHealthAttribute(), InitialHealth);
		ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), InitialHealth);
	}

	PlaceOnSide(StartSide, ChooseLinePosition(StartSide));
	UE_LOG(LogTemp, Log, TEXT("[Daewa] %s aparece fuera del tablero con %.0f de vida (bajara a %.0f al empezar el combate)."),
		*GetName(), InitialHealth, HealthAfterDeploy);
}

void ADaewaBoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Espera a que acabe el despliegue para bajar la vida
	if (bDeployDropDone) return;

	const AGameManager* GM = Cast<AGameManager>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr);
	const UBattleSceneManager* BSM = GM ? GM->GetBattleSceneManager() : nullptr;
	if (BSM && BSM->CurrentPhase == EScenePhase::Combat)
	{
		DaewaApplyDeployHealthDrop();
	}
}

void ADaewaBoss::DaewaApplyDeployHealthDrop()
{
	if (bDeployDropDone) return;
	bDeployDropDone = true;
	SetActorTickEnabled(false);

	const float OldHealth = GetCurrentHealth();
	const float NewHealth = FMath::Clamp(HealthAfterDeploy, 1.f, GetMaxHealth());
	SetHealthValue(NewHealth);

	UE_LOG(LogTemp, Log, TEXT("[Daewa] Empieza el combate: la vida baja de %.0f a %.0f."), OldHealth, NewHealth);
	OnDaewaDeployHealthDrop(OldHealth, NewHealth);
}

//Turno
void ADaewaBoss::MoveTowardClosesPlayer()
{
	// Daewa no va por el tablero
	// todo su turno va en MakeAction
}

void ADaewaBoss::MakeAction()
{
	//Sin BT el director llama aqui
	DaewaDoTurn();
	DaewaFinishIfDead();
}

void ADaewaBoss::OnOwnTurnFinished()
{
	Super::OnOwnTurnFinished();

	//Con BT: desaparece en el siguiente frame
	//cuando el turno ya se ha cerrado bien
	if (bPendingDeath)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ADaewaBoss::Die);
	}
}

void ADaewaBoss::DaewaDoTurn()
{
	if (bPendingDeath) return;

	if (!UEgnisCombatLibrary::CanAct(this))
	{
		UE_LOG(LogTemp, Log, TEXT("[Daewa] Esta aturdido: pierde el turno."));
		return;
	}

	++TurnsSinceLastAttack;
	UE_LOG(LogTemp, Log, TEXT("[Daewa] --- Turno de Daewa (vida %.0f) ---"), GetCurrentHealth());

	//Cae el ataque avisado el turno anterior
	if (PendingAttackTiles.Num() > 0)
	{
		DaewaFireLineAttack();
	}

	//Rellena hasta MaxEnemiesOnBoard
	DaewaSummonUpToMax();
	if (bPendingDeath) return;

	//Pasa al siguiente lado
	if (UEgnisCombatLibrary::CanMove(this))
	{
		DaewaMoveToNextSide();
	}

	//Ataque: avisa para el turno siguiente
	//o pega ya si no hay aviso
	if (bWarnBeforeLineAttack)
	{
		if (PendingAttackTiles.Num() == 0 && TurnsSinceLastAttack + 1 >= AttackEveryNTurns)
		{
			DaewaMarkLineAttack();
		}
	}
	else if (TurnsSinceLastAttack >= AttackEveryNTurns)
	{
		DaewaFireLineAttack();
	}
}

void ADaewaBoss::DaewaFinishIfDead()
{
	if (bPendingDeath)
	{
		Die();
	}
}

void ADaewaBoss::Die()
{
	if (!IsValid(this)) return;

	UE_LOG(LogTemp, Log, TEXT("[Daewa] Se ha quedado sin vida y desaparece. Quedan %d enemigos en el tablero."), CountEnemiesOnBoard());
	ClearAttackDebug();

	UWorld* World = GetWorld();
	HandleDeath();

	//Por si ya no quedaba ningun enemigo: que el combate compruebe si se ha ganado
	if (UBattleManager* BM = GetDaewaBattleManager(World))
	{
		BM->UpdateUnitsAlive();
	}
}

//Vida

void ADaewaBoss::SetHealthValue(float NewHealth)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), FMath::Max(0.f, NewHealth));
	}
}

void ADaewaBoss::PayHealth(float Cost)
{
	const float NewHealth = FMath::Max(0.f, GetCurrentHealth() - Cost);
	SetHealthValue(NewHealth);

	if (NewHealth <= 0.f)
	{
		bPendingDeath = true;
		UE_LOG(LogTemp, Log, TEXT("[Daewa] Llega a 0 de vida: deja de invocar y desaparece al acabar su turno."));
	}
}

//Invocador

int32 ADaewaBoss::CountEnemiesOnBoard() const
{
	int32 Count = 0;
	for (TActorIterator<AEnemy> It(GetWorld()); It; ++It)
	{
		if (*It != this && IsValid(*It) && !It->IsActorBeingDestroyed() && It->GetCurrentHealth() > 0.f)
		{
			++Count;
		}
	}
	return Count;
}

const FDaewaSummonEntry* ADaewaBoss::PickSummonEntry() const
{
	int32 TotalWeight = 0;
	for (const FDaewaSummonEntry& Entry : SummonList)
	{
		if (Entry.EnemyClass && Entry.Weight > 0) TotalWeight += Entry.Weight;
	}
	if (TotalWeight <= 0) return nullptr;

	int32 Roll = FMath::RandRange(1, TotalWeight);
	for (const FDaewaSummonEntry& Entry : SummonList)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0) continue;
		Roll -= Entry.Weight;
		if (Roll <= 0) return &Entry;
	}
	return nullptr;
}

bool ADaewaBoss::PickSummonTile(FTileCoord& OutTile) const
{
	if (!Board) return false;

	TArray<ACharacterBase*> Heroes;
	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		if (IsDaewaLiveHero(*It)) Heroes.Add(*It);
	}

	//Casillas libres y su distancia al heroe mas cercano
	TArray<TPair<FTileCoord, int32>> Free;
	for (int32 X = 0; X < Board->BoardSizeX; ++X)
	{
		for (int32 Y = 0; Y < Board->BoardSizeY; ++Y)
		{
			const FTileCoord Tile(X, Y);
			if (Board->IsTileOccupied(Tile)) continue; //ocupada, vacia u obstaculo

			int32 Nearest = MAX_int32;
			for (const ACharacterBase* Hero : Heroes)
			{
				Nearest = FMath::Min(Nearest, UEnemyCombatLibrary::TileDistance(Tile, Hero->CurrentTile));
			}
			Free.Add(TPair<FTileCoord, int32>(Tile, Heroes.Num() > 0 ? Nearest : 0));
		}
	}
	if (Free.Num() == 0) return false;

	//Respeta la distancia minima a los heroes si se puede
	TArray<TPair<FTileCoord, int32>> Allowed = Free.FilterByPredicate(
		[this](const TPair<FTileCoord, int32>& P) { return MinDistanceToHeroes <= 0 || P.Value >= MinDistanceToHeroes; });
	if (Allowed.Num() == 0) Allowed = Free;

	if (SummonPlacement == EDaewaSummonPlacement::Random)
	{
		OutTile = Allowed[FMath::RandRange(0, Allowed.Num() - 1)].Key;
		return true;
	}

	//Cerca/lejos: se queda con las mejores y elige una al azar entre ellas
	const bool bNear = SummonPlacement == EDaewaSummonPlacement::NearHeroes;
	int32 Best = bNear ? MAX_int32 : MIN_int32;
	for (const TPair<FTileCoord, int32>& P : Allowed)
	{
		Best = bNear ? FMath::Min(Best, P.Value) : FMath::Max(Best, P.Value);
	}
	TArray<FTileCoord> BestTiles;
	for (const TPair<FTileCoord, int32>& P : Allowed)
	{
		if (P.Value == Best) BestTiles.Add(P.Key);
	}
	OutTile = BestTiles[FMath::RandRange(0, BestTiles.Num() - 1)];
	return true;
}

AEnemy* ADaewaBoss::SpawnSummon(TSubclassOf<AEnemy> EnemyClass, const FTileCoord& Tile)
{
	if (!Board || !EnemyClass) return nullptr;

	
	const FTransform SpawnTransform(FRotator::ZeroRotator, Board->TileToWorldCenter(Tile));
	AEnemy* NewEnemy = GetWorld()->SpawnActorDeferred<AEnemy>(EnemyClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!NewEnemy) return nullptr;

	NewEnemy->Board = Board;
	NewEnemy->CurrentTile = Tile;
	NewEnemy->FinishSpawning(FTransform(FRotator::ZeroRotator, NewEnemy->GetStandLocationForTile(Tile)));

	//El combate tiene que contarlo (turnos y victoria)
	if (UBattleManager* BM = GetDaewaBattleManager(GetWorld()))
	{
		BM->RegisterUnit(NewEnemy);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Daewa] No hay BattleManager: %s no cuenta en el combate (¿has pulsado Start Battle?)."), *NewEnemy->GetName());
	}
	return NewEnemy;
}

int32 ADaewaBoss::DaewaSummonUpToMax()
{
	if (bPendingDeath || !Board) return 0;
	if (SummonList.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Daewa] SummonList esta vacia: no puede invocar (rellenala en el BP)."));
		return 0;
	}

	int32 Summoned = 0;
	for (int32 i = 0; i < MaxEnemiesOnBoard && !bPendingDeath; ++i)
	{
		if (CountEnemiesOnBoard() >= MaxEnemiesOnBoard) break;

		const FDaewaSummonEntry* Entry = PickSummonEntry();
		if (!Entry) break;

		FTileCoord Tile;
		if (!PickSummonTile(Tile))
		{
			UE_LOG(LogTemp, Log, TEXT("[Daewa] No quedan casillas libres para invocar."));
			break;
		}

		AEnemy* NewEnemy = SpawnSummon(Entry->EnemyClass, Tile);
		if (!NewEnemy) break;

		++Summoned;
		PayHealth(Entry->HealthCost);
		UE_LOG(LogTemp, Log, TEXT("[Daewa] Invoca %s en (%d,%d): pierde %.0f de vida (le queda %.0f)."),
			*NewEnemy->GetName(), Tile.X, Tile.Y, Entry->HealthCost, GetCurrentHealth());
		OnDaewaSummoned(NewEnemy, Entry->HealthCost);
		NotifyAction(EEnemyActionType::Summon, NewEnemy, Entry->HealthCost);
	}

	UE_LOG(LogTemp, Log, TEXT("[Daewa] Enemigos en el tablero: %d de %d (ha invocado %d)."), CountEnemiesOnBoard(), MaxEnemiesOnBoard, Summoned);
	return Summoned;
}

//Movimiento por el borde
int32 ADaewaBoss::GetSideLength(EDaewaSide Side) const
{
	if (!Board) return 0;
	// En los lados X el ataque baja por X y su ancho va por Y (y al reves)
	return (Side == EDaewaSide::XMax || Side == EDaewaSide::XMin) ? Board->BoardSizeY : Board->BoardSizeX;
}

int32 ADaewaBoss::ChooseLinePosition(EDaewaSide Side) const
{
	const int32 MaxPos = FMath::Max(0, GetSideLength(Side) - 2);
	const float CenterPos = MaxPos * 0.5f;

	switch (LinePlacement)
	{
	case EDaewaLinePlacement::Center:
		return MaxPos / 2;
	case EDaewaLinePlacement::Random:
		return FMath::RandRange(0, MaxPos);
	default:
		break;
	}

	//Delante de los heroes: donde la franja de 2 pille a mas
	//Si empatan, la mas centrada
	const bool bXSide = (Side == EDaewaSide::XMax || Side == EDaewaSide::XMin);
	int32 BestPos = MaxPos / 2;
	int32 BestCount = 0;
	float BestCenterDist = TNumericLimits<float>::Max();
	for (int32 Pos = 0; Pos <= MaxPos; ++Pos)
	{
		int32 Count = 0;
		for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
		{
			if (!IsDaewaLiveHero(*It)) continue;
			const int32 Coord = bXSide ? It->CurrentTile.Y : It->CurrentTile.X;
			if (Coord == Pos || Coord == Pos + 1) ++Count;
		}
		const float CenterDist = FMath::Abs(Pos - CenterPos);
		if (Count > BestCount || (Count == BestCount && Count > 0 && CenterDist < BestCenterDist))
		{
			BestCount = Count;
			BestPos = Pos;
			BestCenterDist = CenterDist;
		}
	}
	return BestPos;//sin heroes: el centro
}

void ADaewaBoss::PlaceOnSide(EDaewaSide Side, int32 Position)
{
	CurrentSide = Side;
	CurrentPosition = Position;
	if (!Board) return;

	//Las 2 casillas de fuera que tiene delante la franja de ataque
	const int32 Dist = FMath::Max(1, DistanceOutsideBoard);
	FTileCoord A, B;
	switch (Side)
	{
	case EDaewaSide::XMax: A = FTileCoord(Board->BoardSizeX - 1 + Dist, Position); B = FTileCoord(A.X, Position + 1); break;
	case EDaewaSide::XMin: A = FTileCoord(-Dist, Position);                        B = FTileCoord(A.X, Position + 1); break;
	case EDaewaSide::YMax: A = FTileCoord(Position, Board->BoardSizeY - 1 + Dist); B = FTileCoord(Position + 1, A.Y); break;
	default:               A = FTileCoord(Position, -Dist);                        B = FTileCoord(Position + 1, A.Y); break;
	}

	FVector Location = (Board->TileToWorldCenter(A) + Board->TileToWorldCenter(B)) * 0.5f;
	Location.Z = BaseZ;
	const FVector BoardCenter = Board->TileToWorldCenter(FTileCoord(Board->BoardSizeX / 2, Board->BoardSizeY / 2));
	const FRotator FacingBoard = (BoardCenter - Location).GetSafeNormal2D().Rotation();
	SetActorLocationAndRotation(Location, FacingBoard);

	OnDaewaSideChanged(Side, Position);
}

bool ADaewaBoss::DaewaMoveToNextSide()
{
	int32 Index = 0;
	for (int32 i = 0; i < 4; ++i)
	{
		if (DaewaSideOrder[i] == CurrentSide) Index = i;
	}
	const EDaewaSide NextSide = DaewaSideOrder[bReverseDirection ? (Index + 3) % 4 : (Index + 1) % 4];
	const int32 Position = ChooseLinePosition(NextSide);
	PlaceOnSide(NextSide, Position);

	UE_LOG(LogTemp, Log, TEXT("[Daewa] Pasa al %s, posicion %d (ataca las filas/columnas %d y %d)."),
		*UEnum::GetDisplayValueAsText(NextSide).ToString(), Position, Position, Position + 1);
	return true;
}

//Ataque linea

void ADaewaBoss::GetLineTiles(EDaewaSide Side, int32 Position, TArray<FTileCoord>& OutTiles) const
{
	OutTiles.Reset();
	if (!Board) return;

	const bool bXSide = (Side == EDaewaSide::XMax || Side == EDaewaSide::XMin);
	const int32 Length = bXSide ? Board->BoardSizeX : Board->BoardSizeY;
	for (int32 Along = 0; Along < Length; ++Along)
	{
		for (int32 Width = Position; Width <= Position + 1; ++Width)
		{
			const FTileCoord Tile = bXSide ? FTileCoord(Along, Width) : FTileCoord(Width, Along);
			if (Board->IsInside(Tile)) OutTiles.Add(Tile);
		}
	}
}

void ADaewaBoss::ClearAttackDebug()
{
	if (bDrawDebugLineAttack && GetWorld())
	{
		FlushPersistentDebugLines(GetWorld());
	}
}

bool ADaewaBoss::DaewaMarkLineAttack()
{
	GetLineTiles(CurrentSide, CurrentPosition, PendingAttackTiles);
	if (PendingAttackTiles.Num() == 0) return false;

	UE_LOG(LogTemp, Log, TEXT("[Daewa] AVISO: el proximo turno ataca %d casillas (filas/columnas %d y %d desde el %s)."),
		PendingAttackTiles.Num(), CurrentPosition, CurrentPosition + 1, *UEnum::GetDisplayValueAsText(CurrentSide).ToString());

	if (bDrawDebugLineAttack && Board)
	{
		const FVector Extent(Board->TileSize * 0.45f, Board->TileSize * 0.45f, 5.f);
		for (const FTileCoord& Tile : PendingAttackTiles)
		{
			DrawDebugBox(GetWorld(), Board->TileToWorldCenter(Tile) + FVector(0.f, 0.f, 10.f), Extent, FColor::Red, true, -1.f, 0, 4.f);
		}
	}

	OnDaewaLineAttackMarked(PendingAttackTiles);
	NotifyAction(EEnemyActionType::LineAttackWarning, nullptr, PendingAttackTiles.Num());
	return true;
}

float ADaewaBoss::DaewaFireLineAttack()
{
	// Sin aviso previo: ataca lo que tiene delante ahora
	if (PendingAttackTiles.Num() == 0)
	{
		GetLineTiles(CurrentSide, CurrentPosition, PendingAttackTiles);
	}
	const TArray<FTileCoord> Tiles = PendingAttackTiles;
	PendingAttackTiles.Reset();
	TurnsSinceLastAttack = 0;
	ClearAttackDebug();

	const TSet<FTileCoord> TileSet(Tiles);
	TArray<ACharacterBase*> Targets;
	for (TActorIterator<ACharacterBase> It(GetWorld()); It; ++It)
	{
		ACharacterBase* Character = *It;
		if (!IsValid(Character) || Character == this || Character->GetCurrentHealth() <= 0.f) continue;
		const bool bHero = Character->GetTeam() == 0;
		if (!bHero && !bDamagesOwnEnemies) continue;
		if (TileSet.Contains(Character->CurrentTile)) Targets.Add(Character);
	}

	if (Targets.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Daewa] El ataque cae en %d casillas pero no pilla a nadie."), Tiles.Num());
		OnDaewaLineAttackFired(Tiles, 0.f);
		NotifyAction(EEnemyActionType::LineAttack, nullptr, 0.f);
		return 0.f;
	}

	const float Rolled = UEnemyCombatLibrary::RollValue(LineAttackPower) * GetDamageMultiplier();
	float Total = 0.f;
	for (ACharacterBase* Target : Targets)
	{
		if (!IsValid(Target)) continue;
		const FString TargetName = Target->GetName();
		const float Damage = UEgnisCombatLibrary::CalculateDamage(this, Target, Rolled);
		if (!ApplyDaewaDamage(Target, AttackEffect, Damage, this))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Daewa] No tiene Attack Effect (GE_CardDamage) asignado en el BP."));
			break;
		}
		Total += Damage;
		UE_LOG(LogTemp, Log, TEXT("[Daewa]   %s recibe %.0f (vida %.0f)."), *TargetName, Damage,
			IsValid(Target) ? Target->GetCurrentHealth() : 0.f);
	}

	if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>())
	{
		AM->PlayTrigger(EEgnisAudioTrigger::EnemyAttack, nullptr, this);
	}

	UE_LOG(LogTemp, Log, TEXT("[Daewa] Ataque en linea: %d golpeados, %.0f de daño total."), Targets.Num(), Total);
	OnDaewaLineAttackFired(Tiles, Total);
	NotifyAction(EEnemyActionType::LineAttack, nullptr, Total);
	return Total;
}