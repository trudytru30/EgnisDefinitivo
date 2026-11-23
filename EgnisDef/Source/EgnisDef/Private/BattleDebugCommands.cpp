// Comandos de consola SOLO PARA PRUEBAS del combate.
//   Egnis.Battle.End win    -> termina el combate como victoria
//   Egnis.Battle.End lose   -> termina el combate como derrota (la fogata pierde una llama)
// Pasan por BattleManager::EndBattle, el mismo camino que el final real del combate.

#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/GameManager.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisBattleCmd, Log, All);

static FAutoConsoleCommandWithWorldAndArgs GEgnisBattleEndCmd(
	TEXT("Egnis.Battle.End"),
	TEXT("Termina el combate actual. Uso: Egnis.Battle.End win | lose"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const AGameManager* GM = World ? Cast<AGameManager>(World->GetAuthGameMode()) : nullptr;
		const UBattleSceneManager* BSM = GM ? GM->GetBattleSceneManager() : nullptr;
		UBattleManager* BM = BSM ? BSM->GetBattleManager() : nullptr;
		if (!BM)
		{
			UE_LOG(LogEgnisBattleCmd, Warning, TEXT("Egnis.Battle.End: no hay combate en este nivel."));
			return;
		}

		const bool bWin = !Args.IsValidIndex(0) || !Args[0].Equals(TEXT("lose"), ESearchCase::IgnoreCase);
		UE_LOG(LogEgnisBattleCmd, Log, TEXT("Egnis.Battle.End: terminando el combate (%s)."), bWin ? TEXT("victoria") : TEXT("derrota"));
		BM->EndBattle(bWin);
	}));