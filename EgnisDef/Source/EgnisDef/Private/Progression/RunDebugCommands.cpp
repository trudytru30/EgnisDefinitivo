
//Funcionan en cualquier mapa mientras el juego está en Play
//Para escribirlos: Play -> clic en la ventana del juego -> tecla ' º' -> comando -> Enter
//Resultados en el Output Log, filtrando por "LogEgnis"
//La mochila vive en memoria

#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "Characters/HeroDataAsset.h"
#include "Cards/BaseCard.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogEgnisRunCmd, Log, All);

namespace
{
	//Tamaño del mazo de prueba (set inicial del GDDcon 20 cartas)
	constexpr int32 DebugDeckSize = 20;

	URunProgressionSubsystem* GetRunSubsystem(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		URunProgressionSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunProgressionSubsystem>() : nullptr;
		if (!Run)
		{
			UE_LOG(LogEgnisRunCmd, Warning, TEXT("[Run] Comando ignorado: el juego tiene que estar en Play."));
		}
		return Run;
	}

	//Busca en el proyecto todos los assets de un tipo (héroes o cartas), ordenados por nombre
	template <typename AssetType>
	TArray<AssetType*> FindAllRunAssets()
	{
		TArray<FAssetData> AssetDataList;
		IAssetRegistry::GetChecked().GetAssetsByClass(AssetType::StaticClass()->GetClassPathName(), AssetDataList, true);

		TArray<AssetType*> Assets;
		for (const FAssetData& AssetData : AssetDataList)
		{
			if (AssetType* Asset = Cast<AssetType>(AssetData.GetAsset()))
			{
				Assets.Add(Asset);
			}
		}
		Assets.Sort([](const AssetType& A, const AssetType& B) { return A.GetName() < B.GetName(); });
		return Assets;
	}

	//Busca un asset por su nombre (ej "DA_HealCard")
	//Si no existe, dice en el log cuáles hay
	template <typename AssetType>
	AssetType* FindRunAssetByName(const FString& AssetName)
	{
		const TArray<AssetType*> Assets = FindAllRunAssets<AssetType>();
		TArray<FString> AvailableNames;
		for (AssetType* Asset : Assets)
		{
			if (Asset->GetName().Equals(AssetName, ESearchCase::IgnoreCase))
			{
				return Asset;
			}
			AvailableNames.Add(Asset->GetName());
		}

		UE_LOG(LogEgnisRunCmd, Warning, TEXT("[Run] No existe '%s'. Disponibles: %s"),
			*AssetName, *FString::Join(AvailableNames, TEXT(", ")));
		return nullptr;
	}

	//Comprueba que el comando tiene las palabras que necesita
	//si no, enseña cómo se usa
	bool HasArgs(const TArray<FString>& Args, int32 Count, const TCHAR* Usage)
	{
		if (Args.Num() >= Count) return true;
		UE_LOG(LogEgnisRunCmd, Warning, TEXT("[Run] Uso: %s"), Usage);
		return false;
	}

	//Envuelve cada comando: consigue la mochila
	//si existe, ejecuta lo que toca
	FConsoleCommandWithWorldAndArgsDelegate RunCommand(TFunction<void(URunProgressionSubsystem&, const TArray<FString>&)> Body)
	{
		return FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([Body](const TArray<FString>& Args, UWorld* World)
		{
			if (URunProgressionSubsystem* Run = GetRunSubsystem(World))
			{
				Body(*Run, Args);
			}
		});
	}

	//Ciclo runs

	FAutoConsoleCommandWithWorldAndArgs CmdRunStart(
		TEXT("Egnis.Run.Start"),
		TEXT("Empieza una run de prueba. Uso: Egnis.Run.Start [Heroe1 Heroe2 ...] (sin nombres = todos)"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			TArray<UHeroDataAsset*> Heroes;
			if (Args.Num() == 0)
			{
				Heroes = FindAllRunAssets<UHeroDataAsset>();
			}
			else
			{
				for (const FString& HeroName : Args)
				{
					if (UHeroDataAsset* Hero = FindRunAssetByName<UHeroDataAsset>(HeroName))
					{
						Heroes.Add(Hero);
					}
				}
			}

			//Mazo de prueba :todas las cartas del proyecto, repetidas hasta llegar a DebugDeckSize
			const TArray<UBaseCard*> AllCards = FindAllRunAssets<UBaseCard>();
			TArray<UBaseCard*> StartingDeck;
			for (int32 Index = 0; AllCards.Num() > 0 && StartingDeck.Num() < DebugDeckSize; ++Index)
			{
				StartingDeck.Add(AllCards[Index % AllCards.Num()]);
			}

			Run.StartNewRun(Heroes, StartingDeck);
			Run.LogRun();
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunDump(
		TEXT("Egnis.Run.Dump"),
		TEXT("Muestra el contenido de la mochila de la run."),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			Run.LogRun();
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunEnd(
		TEXT("Egnis.Run.End"),
		TEXT("Termina la run. Uso: Egnis.Run.End 1 (victoria) / Egnis.Run.End 0 (derrota)"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.End <1|0>"))) return;
			Run.EndRun(FCString::Atoi(*Args[0]) != 0);
		}));

	//Héroes y heridas

	FAutoConsoleCommandWithWorldAndArgs CmdRunAddHero(
		TEXT("Egnis.Run.AddHero"),
		TEXT("Anade un heroe a la caravana. Uso: Egnis.Run.AddHero DA_Tothum"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.AddHero <Heroe>"))) return;
			if (UHeroDataAsset* Hero = FindRunAssetByName<UHeroDataAsset>(Args[0]))
			{
				Run.AddHeroToRun(Hero, true);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunWound(
		TEXT("Egnis.Run.Wound"),
		TEXT("Pone una herida aleatoria a un heroe. Uso: Egnis.Run.Wound DA_Tothum"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.Wound <Heroe>"))) return;
			Run.AddRandomWound(FName(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunHeal(
		TEXT("Egnis.Run.Heal"),
		TEXT("Cura la herida mas antigua de un heroe. Uso: Egnis.Run.Heal DA_Tothum"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.Heal <Heroe>"))) return;
			Run.HealOneWound(FName(*Args[0]));
		}));

	//Huesos y fogata

	FAutoConsoleCommandWithWorldAndArgs CmdRunAddBones(
		TEXT("Egnis.Run.AddBones"),
		TEXT("Suma huesos. Uso: Egnis.Run.AddBones 500"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.AddBones <N>"))) return;
			Run.AddBones(FCString::Atoi(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunSpendBones(
		TEXT("Egnis.Run.SpendBones"),
		TEXT("Gasta huesos. Uso: Egnis.Run.SpendBones 750"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.SpendBones <N>"))) return;
			Run.SpendBones(FCString::Atoi(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunLoseFire(
		TEXT("Egnis.Run.LoseFire"),
		TEXT("Resta una llama a la fogata (como perder un combate)."),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			Run.LoseCampfire();
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunRestoreFire(
		TEXT("Egnis.Run.RestoreFire"),
		TEXT("Recupera llamas de la fogata. Uso: Egnis.Run.RestoreFire [N] (por defecto 1)"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			Run.RestoreCampfire(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1);
		}));

	//Reliquias

	FAutoConsoleCommandWithWorldAndArgs CmdRunAddRelic(
		TEXT("Egnis.Run.AddRelic"),
		TEXT("Mete una reliquia en la bolsa. Uso: Egnis.Run.AddRelic Rel_Boomerang"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.AddRelic <Reliquia>"))) return;
			Run.AddRelic(FName(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunEquip(
		TEXT("Egnis.Run.Equip"),
		TEXT("Equipa una reliquia de la bolsa a un heroe. Uso: Egnis.Run.Equip DA_Tothum Rel_Boomerang"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 2, TEXT("Egnis.Run.Equip <Heroe> <Reliquia>"))) return;
			Run.EquipRelic(FName(*Args[0]), FName(*Args[1]));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunUnequip(
		TEXT("Egnis.Run.Unequip"),
		TEXT("Devuelve una reliquia del heroe a la bolsa. Uso: Egnis.Run.Unequip DA_Tothum Rel_Boomerang"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 2, TEXT("Egnis.Run.Unequip <Heroe> <Reliquia>"))) return;
			Run.UnequipRelic(FName(*Args[0]), FName(*Args[1]));
		}));

	//Mazo

	FAutoConsoleCommandWithWorldAndArgs CmdRunAddCard(
		TEXT("Egnis.Run.AddCard"),
		TEXT("Anade una carta al mazo. Uso: Egnis.Run.AddCard DA_HealCard"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.AddCard <Carta>"))) return;
			if (UBaseCard* Card = FindRunAssetByName<UBaseCard>(Args[0]))
			{
				Run.AddCardToDeck(Card);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunRemoveCard(
		TEXT("Egnis.Run.RemoveCard"),
		TEXT("Quita la carta de esa posicion del mazo (empieza en 0). Uso: Egnis.Run.RemoveCard 3"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.RemoveCard <Posicion>"))) return;
			if (!Run.RemoveCardFromDeck(FCString::Atoi(*Args[0])))
			{
				UE_LOG(LogEgnisRunCmd, Warning, TEXT("[Run] No hay ninguna carta en la posicion %s."), *Args[0]);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdRunUpgrade(
		TEXT("Egnis.Run.Upgrade"),
		TEXT("Mejora la carta de esa posicion del mazo (empieza en 0). Uso: Egnis.Run.Upgrade 0"),
		RunCommand([](URunProgressionSubsystem& Run, const TArray<FString>& Args)
		{
			if (!HasArgs(Args, 1, TEXT("Egnis.Run.Upgrade <Posicion>"))) return;
			if (!Run.UpgradeCard(FCString::Atoi(*Args[0])))
			{
				UE_LOG(LogEgnisRunCmd, Warning, TEXT("[Run] No hay ninguna carta en la posicion %s."), *Args[0]);
			}
		}));
}
// COMANDOS
//   Egnis.Run.Start [Heroe1 Heroe2 ...]   -> empieza una run (sin nombres = todos los héroes)
//                                           con un mazo de prueba de 20 cartas
//   Egnis.Run.Dump                        -> muestra toda la mochila
//   Egnis.Run.End <1|0>                   -> termina la run (1 = victoria, 0 = derrota)
//   Egnis.Run.AddHero <Heroe>             -> añade un héroe a la caravana
//   Egnis.Run.AddBones <N> / SpendBones <N>
//   Egnis.Run.LoseFire / RestoreFire [N]
//   Egnis.Run.Wound <Heroe> / Heal <Heroe>
//   Egnis.Run.AddRelic <Reliquia>
//   Egnis.Run.Equip <Heroe> <Reliquia> / Unequip <Heroe> <Reliquia>
//   Egnis.Run.AddCard <Carta> / RemoveCard <Posicion> / Upgrade <Posicion>
//   (Las posiciones del mazo empiezan en 0. Si un nombre no existe, el log dice los disponibles)
//
// PRUEBAS
// A1. La mochila sobrevive al cambio de mapa
//   1. Play en L_Test_A y escribir:
//        Egnis.Run.Start DA_Tothüm DA_Jhünn
//        Egnis.Run.AddBones 500
//        Egnis.Run.Wound DA_Tothüm
//        Egnis.Run.LoseFire
//        Egnis.Run.AddRelic Rel_Boomerang
//        Egnis.Run.Equip DA_Tothüm Rel_Boomerang
//        Egnis.Run.AddCard DA_HealCard
//        Egnis.Run.Upgrade 0
//   2. open L_Test_B
//   3. Egnis.Run.Dump
//   -> 500 huesos, fogata 2/3, Tothüm con 1 herida y Rel_Boomerang, 21 cartas (la primera con +1)
//
// A2. Máximo 3 heridas sin repetir
//   Egnis.Run.Wound DA_Jhünn cuatro veces
//   -> 3 heridas distintas; la cuarta dice "ya tiene el maximo de heridas"
//
// A3. No se gastan huesos que no hay
//   Egnis.Run.SpendBones 9999
//   -> "No hay huesos suficientes" y los huesos no cambian
//
// A4. Reliquias: límites
//   Egnis.Run.Equip DA_Jhünn Rel_NoExiste            -> "no esta en la bolsa"
//   Con 2 reliquias equipadas, equipar una tercera     -> "no tiene hueco"
//
// A5. Fogata a 0 termina la run (y avisa al diario)
//   Egnis.Run.LoseFire hasta llegar a 0
//   -> "La fogata se ha apagado" y "Run terminada: DERROTA"
//   Egnis.Profile.Dump -> las runs empezadas han subido en 1
//
// A6. La mochila NO sobrevive a cerrar el juego (es lo esperado)
//   Parar el Play, volver a darle y Egnis.Run.Dump -> run activa: no,no todo vacío
