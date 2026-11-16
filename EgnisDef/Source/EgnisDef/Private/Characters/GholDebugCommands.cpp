// Comandos de consola para probar a Ghöl (miniboss region 1).
// Play -> Start Battle -> tecla º -> comando -> Enter -> mirar Output Log (filtrar por "Ghol")

#include "Characters/GholMiniboss.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Characters/BoardPlayerController.h"

namespace
{
	template <typename FuncType>
	void ForEachGhol(UWorld* World, FuncType Func)
	{
		if (!World) return;
		int32 Count = 0;
		for (TActorIterator<AGholMiniboss> It(World); It; ++It)
		{
			++Count;
			Func(*It);
		}
		if (Count == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GholDebug] No hay ningun Ghol en el nivel (o ya ha muerto)."));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs CmdGholInfo(
		TEXT("Egnis.Ghol.Info"),
		TEXT("Muestra vida, stats, si esta en modo muerte, veces que ha revivido y turnos extra."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachGhol(World, [](AGholMiniboss* Ghol)
			{
				UE_LOG(LogTemp, Log, TEXT("[GholDebug] %s: vida %.0f/%.0f, poder %.1f, movimiento %d, ataque %d | modo muerte: %s (%.1f s) | revivido %d | turnos extra %d."),
					*Ghol->GetName(), Ghol->GetCurrentHealth(), Ghol->GetMaxHealth(), Ghol->GetPower(), Ghol->GetMoveRange(),
					Ghol->GetAttackRange(), Ghol->IsInDeathMode() ? TEXT("SI") : TEXT("no"), Ghol->GetDeathModeSecondsLeft(),
					Ghol->GetReviveCount(), Ghol->GetExtraTurnsLeft());
			});
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdGholHurt(
		TEXT("Egnis.Ghol.Hurt"),
		TEXT("Egnis.Ghol.Hurt [cantidad]: le quita vida (sin escudo). Sin numero le quita toda la barra que tenga."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachGhol(World, [&Args](AGholMiniboss* Ghol)
			{
				const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : Ghol->GetCurrentHealth();
				UE_LOG(LogTemp, Log, TEXT("[GholDebug] %s pierde %.0f de vida."), *Ghol->GetName(), Amount);
				UEgnisCombatLibrary::ApplyPureDamage(Ghol, Amount);
			});
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdGholDrain(
		TEXT("Egnis.Ghol.Drain"),
		TEXT("Egnis.Ghol.Drain [segundos=10]: gasta la segunda barra como si pasaran esos segundos."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.f;
			ForEachGhol(World, [Seconds](AGholMiniboss* Ghol) { Ghol->GholDrainDeathBar(Seconds); });
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdGholRevive(
		TEXT("Egnis.Ghol.Revive"),
		TEXT("Hace revivir a Ghol como si hubiera matado a un heroe (solo funciona en modo muerte)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachGhol(World, [](AGholMiniboss* Ghol)
			{
				if (!Ghol->IsInDeathMode())
				{
					UE_LOG(LogTemp, Warning, TEXT("[GholDebug] %s no esta en modo muerte: usa antes Egnis.Ghol.Hurt."), *Ghol->GetName());
					return;
				}
				Ghol->GholRevive();
			});
		}));

	//Para escenas de prueba sin el boton de pasar turno
	FAutoConsoleCommandWithWorldAndArgs CmdEndTurn(
		TEXT("Egnis.EndTurn"),
		TEXT("Pasa el turno del jugador (igual que el boton de fin de turno)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ABoardPlayerController* PC = World ? Cast<ABoardPlayerController>(World->GetFirstPlayerController()) : nullptr;
			if (!PC)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GholDebug] No hay BoardPlayerController: no se puede pasar turno."));
				return;
			}
			UE_LOG(LogTemp, Log, TEXT("[GholDebug] Pasando turno por consola."));
			PC->RequestEndTurn();
		}));
}
