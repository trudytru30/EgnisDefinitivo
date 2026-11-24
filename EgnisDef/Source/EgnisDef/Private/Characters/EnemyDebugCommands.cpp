// Play -> clic en la ventana del juego -> tecla º -> comando -> enter y mirar registro
//
//   Egnis.Enemy.Debug  -> para cada enemigo: sus estadísticas, a qué héroes puede atacar
//                         ahora mismo (rango en rombo + obstáculos) y una tirada de ejemplo

#include "Characters/Enemy.h"
#include "Characters/EnemyCombatLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "DrawDebugHelpers.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Components/StatusEffectComponent.h"
#include "CoreAndSystems/CombatTimerSubsystem.h"
#include "Engine/GameInstance.h"
namespace
{
	FAutoConsoleCommandWithWorldAndArgs CmdEnemyDebug(
		TEXT("Egnis.Enemy.Debug"),
		TEXT("Muestra estadisticas de cada enemigo y a quien puede atacar ahora mismo."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				return;
			}

			int32 Count = 0;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				AEnemy* Enemy = *It;
				++Count;
				UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s en (%d,%d): vida %.0f/%.0f, poder %.1f, movimiento %d, ataque %d%s"),
					*Enemy->GetName(), Enemy->CurrentTile.X, Enemy->CurrentTile.Y,
					Enemy->GetCurrentHealth(), Enemy->GetMaxHealth(), Enemy->GetPower(),
					Enemy->GetMoveRange(), Enemy->GetAttackRange(),
					Enemy->bAttacksOverObstacles ? TEXT(" (ataca por encima de obstaculos)") : TEXT(""));

				const TArray<ACharacterBase*> Targets = UEnemyCombatLibrary::GetHeroesInAttackRange(Enemy);
				if (Targets.Num() == 0)
				{
					UE_LOG(LogTemp, Log, TEXT("[EnemyDebug]   No puede atacar a nadie ahora mismo."));
				}
				for (const ACharacterBase* Target : Targets)
				{
					UE_LOG(LogTemp, Log, TEXT("[EnemyDebug]   Puede atacar a %s en (%d,%d), a %d casillas."),
						*Target->GetName(), Target->CurrentTile.X, Target->CurrentTile.Y,
						UEnemyCombatLibrary::TileDistance(Enemy->CurrentTile, Target->CurrentTile));
				}

				UEnemyCombatLibrary::RollValue(Enemy->GetPower());
			}

			if (Count == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("[EnemyDebug] No hay enemigos (¿estas en Play?)."));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyAttack(
		TEXT("Egnis.Enemy.Attack"),
		TEXT("Cada enemigo ataca al primer heroe que tenga a tiro."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				const TArray<ACharacterBase*> Targets = UEnemyCombatLibrary::GetHeroesInAttackRange(*It);
				if (Targets.Num() == 0)
				{
					UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s no tiene a nadie a tiro."), *It->GetName());
					continue;
				}
				UEnemyCombatLibrary::EnemyAttack(*It, Targets[0]);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyShield(
		TEXT("Egnis.Enemy.Shield"),
		TEXT("Cada enemigo se pone escudo a si mismo."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				UEnemyCombatLibrary::EnemyShield(*It, nullptr);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyBuff(
		TEXT("Egnis.Enemy.Buff"),
		TEXT("Egnis.Enemy.Buff [porcentaje=10] [turnos=1]: cada enemigo se pone bufo de daño."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			const float Percent = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.f;
			const int32 Turns = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				UEnemyCombatLibrary::EnemyDamageBuff(*It, nullptr, Percent, Turns);
			}
		}));

		//Heroe vivo mas cercano (en casillas) a un enemigo
	ACharacterBase* FindClosestHeroForDebug(const AEnemy* Enemy)
	{
		ACharacterBase* Best = nullptr;
		int32 BestDistance = MAX_int32;
		for (TActorIterator<ACharacterBase> It(Enemy->GetWorld()); It; ++It)
		{
			if (It->GetTeam() != 0 || It->GetCurrentHealth() <= 0.f) continue;
			const int32 Distance = UEnemyCombatLibrary::TileDistance(Enemy->CurrentTile, It->CurrentTile);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		return Best;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyReach(
		TEXT("Egnis.Enemy.Reach"),
		TEXT("Pinta 5 s una bola verde en cada casilla a la que puede llegar cada enemigo."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				const TArray<FTileCoord> Tiles = UEnemyCombatLibrary::GetReachableTiles(*It);
				UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s en (%d,%d) puede llegar a %d casillas (movimiento %d)."),
					*It->GetName(), It->CurrentTile.X, It->CurrentTile.Y, Tiles.Num(), It->GetMoveRange());
				if (!It->Board) continue;
				for (const FTileCoord& Tile : Tiles)
				{
					DrawDebugSphere(World, It->Board->TileToWorldCenter(Tile) + FVector(0.f, 0.f, 60.f),
						25.f, 8, FColor::Green, false, 5.f);
				}
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyMove(
		TEXT("Egnis.Enemy.Move"),
		TEXT("Cada enemigo va a por el heroe mas cercano (se acerca o se pone a tiro)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				if (ACharacterBase* Hero = FindClosestHeroForDebug(*It))
				{
					UEnemyCombatLibrary::EnemyMoveTowards(*It, Hero);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] No hay heroes en el tablero."));
				}
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyMoveTo(
		TEXT("Egnis.Enemy.MoveTo"),
		TEXT("Egnis.Enemy.MoveTo X Y: cada enemigo intenta moverse a esa casilla."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || Args.Num() < 2) return;
			const FTileCoord Target(FCString::Atoi(*Args[0]), FCString::Atoi(*Args[1]));
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				UEnemyCombatLibrary::EnemyMoveToTile(*It, Target);
			}
		}));


	FAutoConsoleCommandWithWorldAndArgs CmdEnemyHurt(
	TEXT("Egnis.Enemy.Hurt"),
	TEXT("Egnis.Enemy.Hurt [cantidad=20]: quita vida a cada enemigo (sin tocar escudo) para probar curas."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 20.f;
		for (TActorIterator<AEnemy> It(World); It; ++It)
		{
			UEgnisCombatLibrary::ApplyPureDamage(*It, Amount);
			UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s pierde %.0f: vida %.0f/%.0f."),
				*It->GetName(), Amount, It->GetCurrentHealth(), It->GetMaxHealth());
		}
	}));
//CURA
	FAutoConsoleCommandWithWorldAndArgs CmdEnemyHeal(
		TEXT("Egnis.Enemy.Heal"),
		TEXT("Egnis.Enemy.Heal [porcentaje=100]: cada enemigo cura al aliado a rango con menos % de vida (por debajo de ese %)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			const float Percent = Args.Num() > 0 ? FCString::Atof(*Args[0]) / 100.f : 1.f;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				if (ACharacterBase* Ally = UEnemyCombatLibrary::FindMostHurtAlly(*It, Percent))
				{
					UEnemyCombatLibrary::EnemyHeal(*It, Ally);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s no tiene a ningun aliado herido a rango."), *It->GetName());
				}
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyArea(
	TEXT("Egnis.Enemy.Area"),
	TEXT("Egnis.Enemy.Area [radio=1] [repartir=0]: cada enemigo lanza un ataque en area donde pille a mas heroes."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		const int32 Radius = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
		const bool bSplit = Args.Num() > 1 && FCString::Atoi(*Args[1]) != 0;
		for (TActorIterator<AEnemy> It(World); It; ++It)
		{
			FTileCoord Center;
			const int32 Hits = UEnemyCombatLibrary::FindBestAreaCenter(*It, Radius, Center);
			if (Hits == 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s no alcanza a ningun heroe con un area de radio %d."), *It->GetName(), Radius);
				continue;
			}
			UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s: mejor centro (%d,%d), pilla a %d heroes."), *It->GetName(), Center.X, Center.Y, Hits);
			UEnemyCombatLibrary::EnemyAreaAttack(*It, Center, Radius, bSplit);
		}
	}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyAttackHP(
		TEXT("Egnis.Enemy.AttackHP"),
		TEXT("Egnis.Enemy.AttackHP [mas=1]: cada enemigo ataca al heroe a tiro con mas vida (0 = con menos vida)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			const bool bMost = Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				ACharacterBase* Target = UEnemyCombatLibrary::FindHeroByHealth(*It, bMost, true);
				if (!Target)
				{
					UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s no tiene a nadie a tiro."), *It->GetName());
					continue;
				}
				UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s elige a %s (vida %.0f)."), *It->GetName(), *Target->GetName(), Target->GetCurrentHealth());
				UEnemyCombatLibrary::EnemyAttack(*It, Target);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyStrike(
	TEXT("Egnis.Enemy.Strike"),
	TEXT("Cada enemigo hace el golpe de Ikwos al heroe a tiro con menos vida (ejecucion / mitad de vida / sangrado)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		for (TActorIterator<AEnemy> It(World); It; ++It)
		{
			ACharacterBase* Target = UEnemyCombatLibrary::FindHeroByHealth(*It, false, true);
			if (!Target)
			{
				UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s no tiene a nadie a tiro."), *It->GetName());
				continue;
			}
			UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s (vida %.0f) va a por %s (vida %.0f/%.0f)."), *It->GetName(),
				It->GetCurrentHealth(), *Target->GetName(), Target->GetCurrentHealth(), Target->GetMaxHealth());
			UEnemyCombatLibrary::EnemyExecutionStrike(*It, Target);
		}
	}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyCenter(
	TEXT("Egnis.Enemy.Center"),
	TEXT("Cada enemigo va lo mas cerca posible del centro de los heroes (movimiento del tanque)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		for (TActorIterator<AEnemy> It(World); It; ++It)
		{
			UEnemyCombatLibrary::EnemyMoveToHeroesCenter(*It);
		}
	}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyLoseShield(
		TEXT("Egnis.Enemy.LoseShield"),
		TEXT("Egnis.Enemy.LoseShield [porcentaje=50]: cada enemigo pierde ese % de su escudo y muestra su vida+escudo."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			const float Percent = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 50.f;
			for (TActorIterator<AEnemy> It(World); It; ++It)
			{
				UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] %s antes: vida %.0f + escudo %.0f = %.0f%%."), *It->GetName(),
					It->GetCurrentHealth(), It->GetCurrentShield(), UEnemyCombatLibrary::GetHealthPlusShieldPercent(*It));
				UEnemyCombatLibrary::EnemyLoseShieldPercent(*It, Percent);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyTaunt(
	TEXT("Egnis.Enemy.Taunt"),
	TEXT("Egnis.Enemy.Taunt [turnos=1] [distancia=0]: cada enemigo provoca a los heroes (0 = todos)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		const int32 Turns = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
		const int32 Distance = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
		for (TActorIterator<AEnemy> It(World); It; ++It)
		{
			UEnemyCombatLibrary::EnemyTaunt(*It, Turns, Distance);
		}
	}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyTauntCheck(
	TEXT("Egnis.Enemy.TauntCheck"),
	TEXT("Muestra que heroes estan provocados, por quien, y a que enemigos podrian apuntar con una carta."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		for (TActorIterator<ACharacterBase> Hero(World); Hero; ++Hero)
		{
			if (Hero->GetTeam() != 0 || !Hero->StatusComp) continue;
			const ACharacterBase* Taunter = Hero->StatusComp->GetStatusSource(EStatusType::Status_Taunt);
			if (!IsValid(Taunter) || Taunter->GetCurrentHealth() <= 0.f)
			{
				UE_LOG(LogTemp, Log, TEXT("[TauntCheck] %s NO esta provocado: puede apuntar a cualquier enemigo."), *Hero->GetName());
				continue;
			}
			UE_LOG(LogTemp, Log, TEXT("[TauntCheck] %s provocado por %s (%d turnos)."), *Hero->GetName(), *Taunter->GetName(),
				Hero->StatusComp->GetTurnsLeft(EStatusType::Status_Taunt));
			for (TActorIterator<AEnemy> Enemy(World); Enemy; ++Enemy)
			{
				UE_LOG(LogTemp, Log, TEXT("[TauntCheck]   -> %s: %s"), *Enemy->GetName(),
					*Enemy == Taunter ? TEXT("SI puede apuntarle") : TEXT("NO (bloqueado por el taunt)"));
			}
		}
	}));
	FAutoConsoleCommandWithWorldAndArgs CmdEnemyProvoke(
	TEXT("Egnis.Enemy.Provoke"),
	TEXT("Egnis.Enemy.Provoke [turnos=2] [indiceEnemigo=0]: ese enemigo aplica Atraccion Espiritual a todos los heroes."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		const int32 Turns = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 2;
		const int32 EnemyIndex = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;

		int32 Index = 0;
		for (TActorIterator<AEnemy> Enemy(World); Enemy; ++Enemy, ++Index)
		{
			if (Index != EnemyIndex) continue;

			for (TActorIterator<ACharacterBase> Hero(World); Hero; ++Hero)
			{
				if (Hero->GetTeam() != 0 || !Hero->StatusComp) continue;
				Hero->StatusComp->ApplyStatus(EStatusType::Status_SpiritAttraction, FMath::Max(1, Turns), *Enemy);
				UE_LOG(LogTemp, Log, TEXT("[ProvokeDebug] %s provoca a %s (%d turnos)."), *Enemy->GetName(), *Hero->GetName(), Turns);
			}
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[ProvokeDebug] No hay enemigo con indice %d."), EnemyIndex);
	}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyProvokeCheck(
	TEXT("Egnis.Enemy.ProvokeCheck"),
	TEXT("Muestra que heroes estan provocados por Atraccion Espiritual, por quien, y a que enemigos podrian apuntar."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		for (TActorIterator<ACharacterBase> Hero(World); Hero; ++Hero)
		{
			if (Hero->GetTeam() != 0 || !Hero->StatusComp) continue;
			const ACharacterBase* Provoker = Hero->StatusComp->GetStatusSource(EStatusType::Status_SpiritAttraction);
			if (!IsValid(Provoker) || Provoker->GetCurrentHealth() <= 0.f)
			{
				UE_LOG(LogTemp, Log, TEXT("[ProvokeCheck] %s NO esta provocado."), *Hero->GetName());
				continue;
			}
			UE_LOG(LogTemp, Log, TEXT("[ProvokeCheck] %s provocado por %s (%d turnos)."), *Hero->GetName(), *Provoker->GetName(),
				Hero->StatusComp->GetTurnsLeft(EStatusType::Status_SpiritAttraction));
			for (TActorIterator<AEnemy> Enemy(World); Enemy; ++Enemy)
			{
				UE_LOG(LogTemp, Log, TEXT("[ProvokeCheck]   -> %s: %s"), *Enemy->GetName(),
					*Enemy == Provoker ? TEXT("SI puede apuntarle") : TEXT("NO (bloqueado)"));
			}
		}
	}));
	FAutoConsoleCommandWithWorldAndArgs CmdEnemyPossess(
TEXT("Egnis.Enemy.Possess"),
TEXT("Egnis.Enemy.Possess [indiceEnemigo=0] [1|0]: marca o desmarca a ese enemigo como poseido por espiritus (Perfumar)."),
FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
{
	if (!World) return;
	const int32 EnemyIndex = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
	const bool bValue = Args.Num() > 1 ? FCString::Atoi(*Args[1]) != 0 : true;

	int32 Index = 0;
	for (TActorIterator<AEnemy> Enemy(World); Enemy; ++Enemy, ++Index)
	{
		if (Index != EnemyIndex) continue;
		Enemy->bPossessedBySpirits = bValue;
		UE_LOG(LogTemp, Log, TEXT("[PerfumeDebug] %s poseido = %s"), *Enemy->GetName(), bValue ? TEXT("SI") : TEXT("NO"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[PerfumeDebug] No hay enemigo con indice %d."), EnemyIndex);
}));

	FAutoConsoleCommandWithWorldAndArgs CmdEnemyPerfume(
TEXT("Egnis.Enemy.Perfume"),
TEXT("Egnis.Enemy.Perfume [turnos=2] [indiceEnemigo=0] [indiceHeroe=0]: perfuma a ese enemigo; ignorara a ese heroe."),
FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
{
	if (!World) return;
	const int32 Turns = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 2;
	const int32 EnemyIndex = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
	const int32 HeroIndex = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 0;

	ACharacterBase* HeroSource = nullptr;
	int32 Index = 0;
	for (TActorIterator<ACharacterBase> Hero(World); Hero; ++Hero)
	{
		if (Hero->GetTeam() != 0) continue;
		if (Index++ == HeroIndex) { HeroSource = *Hero; break; }
	}

	Index = 0;
	for (TActorIterator<AEnemy> Enemy(World); Enemy; ++Enemy, ++Index)
	{
		if (Index != EnemyIndex || !Enemy->StatusComp) continue;
		Enemy->StatusComp->ApplyStatus(EStatusType::Status_Perfume, FMath::Max(1, Turns), HeroSource);
		UE_LOG(LogTemp, Log, TEXT("[PerfumeDebug] %s perfumado (%d turnos), ignorara a %s."),
			*Enemy->GetName(), Turns, HeroSource ? *HeroSource->GetName() : TEXT("nadie"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[PerfumeDebug] No hay enemigo con indice %d."), EnemyIndex);
}));
	FAutoConsoleCommandWithWorldAndArgs CmdMadness(
TEXT("Egnis.Madness"),
TEXT("Egnis.Madness [indiceHeroe=0]: aplica Locura a ese heroe."),
FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
{
	if (!World) return;
	const int32 HeroIndex = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;

	int32 Index = 0;
	for (TActorIterator<ACharacterBase> Hero(World); Hero; ++Hero)
	{
		if (Hero->GetTeam() != 0 || !Hero->StatusComp) continue;
		if (Index++ != HeroIndex) continue;
		Hero->StatusComp->ApplyStatus(EStatusType::Status_Madness, 1);
		UE_LOG(LogTemp, Log, TEXT("[MadnessDebug] %s esta loco."), *Hero->GetName());
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[MadnessDebug] No hay heroe con indice %d."), HeroIndex);
}));
	FAutoConsoleCommandWithWorldAndArgs CmdStatus(
TEXT("Egnis.Status"),
TEXT("Egnis.Status <Estado> [turnos=2] [indiceHeroe=0]: aplica un estado a ese heroe. Sin argumentos lista los estados. Ej: Egnis.Status Burn 3"),
FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
{
	if (!World) return;
	const UEnum* StatusEnum = StaticEnum<EStatusType>();
	if (!StatusEnum) return;

	if (Args.Num() == 0)
	{
		FString All;
		for (int32 i = 0; i < StatusEnum->NumEnums() - 1; ++i)
		{
			All += StatusEnum->GetNameStringByIndex(i).Replace(TEXT("Status_"), TEXT("")) + TEXT(" ");
		}
		UE_LOG(LogTemp, Log, TEXT("[StatusDebug] Estados: %s"), *All);
		return;
	}

	// Acepta "Burn" o "Status_Burn", sin distinguir mayusculas
	int64 Value = INDEX_NONE;
	for (int32 i = 0; i < StatusEnum->NumEnums() - 1; ++i)
	{
		const FString Name = StatusEnum->GetNameStringByIndex(i).Replace(TEXT("Status_"), TEXT(""));
		if (Name.Equals(Args[0].Replace(TEXT("Status_"), TEXT("")), ESearchCase::IgnoreCase))
		{
			Value = StatusEnum->GetValueByIndex(i);
			break;
		}
	}
	if (Value == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusDebug] Estado desconocido '%s'. Escribe Egnis.Status solo para ver la lista."), *Args[0]);
		return;
	}

	const int32 Turns = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 2;
	const int32 HeroIndex = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 0;

	// Lanzador de prueba: el primer enemigo (Asustado y otros estados necesitan un Source)
	ACharacterBase* FirstEnemy = nullptr;
	for (TActorIterator<ACharacterBase> It(World); It; ++It)
	{
		if (It->GetTeam() == 1) { FirstEnemy = *It; break; }
	}

	int32 Index = 0;
	for (TActorIterator<ACharacterBase> Hero(World); Hero; ++Hero)
	{
		if (Hero->GetTeam() != 0 || !Hero->StatusComp) continue;
		if (Index++ != HeroIndex) continue;
		Hero->StatusComp->ApplyStatus(static_cast<EStatusType>(Value), FMath::Max(1, Turns), FirstEnemy);
		UE_LOG(LogTemp, Log, TEXT("[StatusDebug] %s: %s (%d turnos)"), *Hero->GetName(), *Args[0], FMath::Max(1, Turns));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[StatusDebug] No hay heroe con indice %d."), HeroIndex);
}));

	//Pausa o reanuda el temporizador del combate (usa la pausa que ya tiene el temporizador)
FAutoConsoleCommandWithWorldAndArgs CmdTimer(
	TEXT("Egnis.Timer"),
	TEXT("Egnis.Timer 0/1: 0 = pausa el temporizador del combate, 1 = lo reanuda. Sin numero lo cambia."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		UCombatTimerSubsystem* Timer = GI ? GI->GetSubsystem<UCombatTimerSubsystem>() : nullptr;
		if (!Timer)
		{
			UE_LOG(LogTemp, Warning, TEXT("[EnemyDebug] No se encuentra el temporizador del combate."));
			return;
		}

		const bool bTurnOn = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : Timer->IsTimerPaused();
		if (bTurnOn)
		{
			Timer->ResumeTimer(TEXT("Debug"));
		}
		else
		{
			Timer->PauseTimer(TEXT("Debug"));
		}
		UE_LOG(LogTemp, Log, TEXT("[EnemyDebug] Temporizador %s."), bTurnOn ? TEXT("reanudado") : TEXT("en pausa"));
	}));}