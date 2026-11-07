// Comandos de consola para probar a Daewa (boss region 1) sin Behavior Tree.
// Play -> Start Battle -> tecla º -> comando -> Enter -> mirar Output Log (filtrar por "Daewa")

#include "Characters/DaewaBoss.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	template <typename FuncType>
	void ForEachDaewa(UWorld* World, FuncType Func)
	{
		if (!World) return;
		int32 Count = 0;
		for (TActorIterator<ADaewaBoss> It(World); It; ++It)
		{
			++Count;
			Func(*It);
		}
		if (Count == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DaewaDebug] No hay ningun Daewa en el nivel (o ya ha muerto)."));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs CmdDaewaInfo(
		TEXT("Egnis.Boss.Info"),
		TEXT("Muestra vida, lado, enemigos en el tablero y si tiene un ataque avisado."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDaewa(World, [](ADaewaBoss* Boss)
			{
				UE_LOG(LogTemp, Log, TEXT("[DaewaDebug] %s: vida %.0f/%.0f, %s, enemigos %d/%d, ataque avisado: %d casillas."),
					*Boss->GetName(), Boss->GetCurrentHealth(), Boss->GetMaxHealth(),
					*UEnum::GetDisplayValueAsText(Boss->GetCurrentSide()).ToString(),
					Boss->CountEnemiesOnBoard(), Boss->MaxEnemiesOnBoard, Boss->GetPendingAttackTiles().Num());
			});
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDaewaTurn(
		TEXT("Egnis.Boss.Turn"),
		TEXT("Hace un turno completo de Daewa (ataque avisado, invocar, cambiar de lado, avisar)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDaewa(World, [](ADaewaBoss* Boss) { Boss->DaewaDoTurn(); Boss->DaewaFinishIfDead(); });
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDaewaMove(
		TEXT("Egnis.Boss.Move"),
		TEXT("Daewa pasa al siguiente lado."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDaewa(World, [](ADaewaBoss* Boss) { Boss->DaewaMoveToNextSide(); });
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDaewaSummon(
		TEXT("Egnis.Boss.Summon"),
		TEXT("Daewa invoca hasta tener el maximo de enemigos en el tablero."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDaewa(World, [](ADaewaBoss* Boss) { Boss->DaewaSummonUpToMax(); Boss->DaewaFinishIfDead(); });
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDaewaMark(
		TEXT("Egnis.Boss.Mark"),
		TEXT("Daewa marca (avisa) las casillas de su proximo ataque."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDaewa(World, [](ADaewaBoss* Boss) { Boss->DaewaMarkLineAttack(); });
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDaewaFire(
		TEXT("Egnis.Boss.Fire"),
		TEXT("Daewa lanza el ataque en linea (sobre lo marcado, o lo que tiene delante)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDaewa(World, [](ADaewaBoss* Boss) { Boss->DaewaFireLineAttack(); });
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDaewaDeploy(
		TEXT("Egnis.Boss.DeployDrop"),
		TEXT("Fuerza la bajada de vida del despliegue (normalmente se hace sola al empezar el combate)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ForEachDaewa(World, [](ADaewaBoss* Boss) { Boss->DaewaApplyDeployHealthDrop(); });
		}));
}