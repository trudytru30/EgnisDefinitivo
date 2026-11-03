#include "Cards/Card_SummonEffect.h"
#include "Characters/SummonUnit.h"
#include "Characters/HeroDataAsset.h"
#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/GameManager.h"
#include "Engine/World.h"

UCard_SummonEffect::UCard_SummonEffect()
{
	Target = ECardTarget::Tile;
}

void UCard_SummonEffect::Execute_Tile(ACharacterBase* Self, FVector Location)
{
	Super::Execute_Tile(Self, Location);
	
	const AAlly* Caster = Cast<AAlly>(Self);
	if (!Caster || !Self->Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonEffect: quien juega la carta no es un aliado o no tiene tablero."));
		return;
	}

	if (!SummonClass || !SummonData)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonEffect: faltan SummonClass o SummonData en la carta, no se invoca nada."));
		return;
	}

	ABoard* Board = Self->Board;
	FTileCoord Tile;
	if (!Board->WorldPointToTile(Location, Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonEffect: la posicion esta fuera del tablero."));
		return;
	}

	// IsTileOccupied tambien cuenta como ocupadas las casillas fuera del tablero y las Void
	if (Board->IsTileOccupied(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonEffect: la casilla (%d, %d) no esta libre."), Tile.X, Tile.Y);
		return;
	}

	// Spawn diferido, igual que DeployManager: tablero, casilla, datos y vida se ponen ANTES del BeginPlay,
	// que es donde se inicializa desde el DataAsset y se registra en el tablero
	const FTransform SpawnTransform(FRotator::ZeroRotator, Board->TileToWorldCenter(Tile));
	ASummonUnit* Summon = Self->GetWorld()->SpawnActorDeferred<ASummonUnit>(SummonClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Summon)
	{
		return;
	}

	// Minimo 1: con piedad 0 la invocacion nace con 1 de vida (HealthOverride a 0 significa "usa la tabla")
	const float Health = FMath::Max(1.f, Caster->GetCharacterPiety() * PietyMultiplier);
	Summon->Board = Board;
	Summon->CurrentTile = Tile;
	Summon->HeroAsset = SummonData;
	Summon->HealthOverride = Health;
	Summon->SetSummoningSick(bSummonCooldown);
	Summon->FinishSpawning(FTransform(FRotator::ZeroRotator, Summon->GetStandLocationForTile(Tile)));

	// BattleManager arma CharactersOnField al empezar el combate: hay que avisarle de la nueva unidad
	bool bRegistered = false;
	if (const AGameManager* GM = Cast<AGameManager>(Self->GetWorld()->GetAuthGameMode()))
	{
		if (const UBattleSceneManager* BSM = GM->GetBattleSceneManager())
		{
			if (UBattleManager* BM = BSM->GetBattleManager())
			{
				BM->RegisterUnit(Summon);
				bRegistered = true;
			}
		}
	}
	if (!bRegistered)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonEffect: no se encontro el BattleManager, la invocacion no cuenta en el combate."));
	}

	UE_LOG(LogTemp, Log, TEXT("SummonEffect: %s (piedad %.1f x %.1f) invoca %s en (%d, %d) con %.1f de vida, mareo de invocacion: %s"),
		*Caster->GetName(), Caster->GetCharacterPiety(), PietyMultiplier, *Summon->GetName(), Tile.X, Tile.Y,
		Summon->GetMaxHealth(), bSummonCooldown ? TEXT("si") : TEXT("no"));
}

bool UCard_SummonEffect::CanTargetTile(const ACharacterBase* Self, const FTileCoord& Tile) const
{
	if (!Self || !Self->Board)
	{
		return false;
	}
	
	if (Self->Board->IsTileOccupied(Tile))
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonEffect: la casilla (%d, %d) no esta libre, elige otra."), Tile.X, Tile.Y);
		return false;
	}
	return true;
}