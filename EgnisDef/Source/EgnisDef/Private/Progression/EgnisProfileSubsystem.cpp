#include "Progression/EgnisProfileSubsystem.h"
#include "Progression/EgnisProfileSave.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
/*
 *Para probar :
 *Primero mirar linea 214 y siguientes para ver los comandos de consola que se pueden usar
 * y para pruebas mirar los comemtarios del final
 *
 * Mirar si saved esta en el gitignore
 */
DEFINE_LOG_CATEGORY_STATIC(LogEgnisSave, Log, All);//Log para todo lo relacionado con el guardado de partidas

namespace
{
	//nombre del archivo: Saved/SaveGames/Profile.sav
	const FString ProfileSlotName = TEXT("Profile");
	//Arreglo para copia del archivo cuando está roto, para no perderlo: Saved/SaveGames/Profile_corrupt.sav
	const FString CorruptProfileSlotName = TEXT("Profile_corrupt");
	constexpr int32 ProfileUserIndex = 0;

	//Todo .sav de Unreal empieza por las letras "GVAS"
	//Si no las tiene, no es un guardado válido
	//Hay que comprobarlo ANTES de pasárselo a Unreal: con un archivo basura, LoadGameFromSlot
	//no devuelve nullptr SI NO QUE ROMPE EL EDITOR
	bool LooksLikeSaveGameFile(const TArray<uint8>& Data)
	{
		return Data.Num() > 4 && Data[0] == 'G' && Data[1] == 'V' && Data[2] == 'A' && Data[3] == 'S';
	}
}

void UEgnisProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreateProfile();
}

void UEgnisProfileSubsystem::LoadOrCreateProfile()
{
	Profile = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(ProfileSlotName, ProfileUserIndex))
	{
		//Lee los bytes del archivo  y comprueba la firma antes de dárselo a Unreal
		TArray<uint8> FileData;
		const bool bFileRead = UGameplayStatics::LoadDataFromSlot(FileData, ProfileSlotName, ProfileUserIndex);

		if (bFileRead && LooksLikeSaveGameFile(FileData))
		{
			//Si es un guardado de otra clase el cast devuelve nullptr
			Profile = Cast<UEgnisProfileSave>(UGameplayStatics::LoadGameFromMemory(FileData));
		}

		if (!Profile)
		{
			UE_LOG(LogEgnisSave, Warning, TEXT("[Profile] El archivo existe pero no se puede leer (corrupto o de otro tipo). Se crea un perfil nuevo."));

			//Guarda una copia del archivo roto antes de que se sobrescriba
			if (bFileRead && FileData.Num() > 0 &&
				UGameplayStatics::SaveDataToSlot(FileData, CorruptProfileSlotName, ProfileUserIndex))
			{
				UE_LOG(LogEgnisSave, Warning, TEXT("[Profile] Copia del archivo roto guardada en %s.sav"), *CorruptProfileSlotName);
			}
		}
		else if (Profile->SaveVersion > UEgnisProfileSave::CurrentSaveVersion)
		{
			UE_LOG(LogEgnisSave, Warning, TEXT("[Profile] El archivo es de una version mas nueva (%d) que el juego (%d). Se intenta usar igualmente."),
				Profile->SaveVersion, UEgnisProfileSave::CurrentSaveVersion);
		}
		else if (Profile->SaveVersion < UEgnisProfileSave::CurrentSaveVersion)
		{
			//Aqui se adaptarán archivos antiguos cuando cambie el formato
			UE_LOG(LogEgnisSave, Log, TEXT("[Profile] Archivo de version antigua (%d), se actualiza a %d."),
				Profile->SaveVersion, UEgnisProfileSave::CurrentSaveVersion);
			Profile->SaveVersion = UEgnisProfileSave::CurrentSaveVersion;
		}
	}

	if (!Profile)
	{
		Profile = Cast<UEgnisProfileSave>(UGameplayStatics::CreateSaveGameObject(UEgnisProfileSave::StaticClass()));
		UE_LOG(LogEgnisSave, Log, TEXT("[Profile] Perfil nuevo creado."));
		SaveProfile();
		return;
	}

	UE_LOG(LogEgnisSave, Log, TEXT("[Profile] Perfil cargado."));
	LogProfile();
}

bool UEgnisProfileSubsystem::SaveProfile()
{
	if (!Profile)
	{
		return false;
	}

	Profile->SaveVersion = UEgnisProfileSave::CurrentSaveVersion;
	const bool bSaved = UGameplayStatics::SaveGameToSlot(Profile, ProfileSlotName, ProfileUserIndex);

	if (bSaved)
	{
		UE_LOG(LogEgnisSave, Log, TEXT("[Profile] Guardado en disco."));
	}
	else
	{
		UE_LOG(LogEgnisSave, Error, TEXT("[Profile] ERROR: no se ha podido guardar en disco."));
	}
	return bSaved;
}

void UEgnisProfileSubsystem::ResetProfile()
{
	Profile = Cast<UEgnisProfileSave>(UGameplayStatics::CreateSaveGameObject(UEgnisProfileSave::StaticClass()));
	UE_LOG(LogEgnisSave, Log, TEXT("[Profile] Perfil reiniciado."));
	SaveProfile();
}

//Héroes desbloqueados permanentemente

void UEgnisProfileSubsystem::UnlockHeroForever(FName HeroId)
{
	if (!Profile || HeroId.IsNone()) return;

	//Add devuelve si ya estaba( guarda solo si ha cambiado algo)
	bool bAlreadyUnlocked = false;
	Profile->UnlockedHeroes.Add(HeroId, &bAlreadyUnlocked);
	if (!bAlreadyUnlocked)
	{
		UE_LOG(LogEgnisSave, Log, TEXT("[Profile] Heroe desbloqueado para siempre: %s"), *HeroId.ToString());
		SaveProfile();
	}
}

bool UEgnisProfileSubsystem::IsHeroUnlockedForever(FName HeroId) const
{
	return Profile && Profile->UnlockedHeroes.Contains(HeroId);
}

//Encuentros

void UEgnisProfileSubsystem::MarkEncounterSeen(FName EncounterId)
{
	if (!Profile || EncounterId.IsNone()) return;

	bool bAlreadySeen = false;
	Profile->SeenEncounters.Add(EncounterId, &bAlreadySeen);
	if (!bAlreadySeen)
	{
		UE_LOG(LogEgnisSave, Log, TEXT("[Profile] Encuentro visto: %s"), *EncounterId.ToString());
		SaveProfile();
	}
}

bool UEgnisProfileSubsystem::HasSeenEncounter(FName EncounterId) const
{
	return Profile && Profile->SeenEncounters.Contains(EncounterId);
}

//Misiones

void UEgnisProfileSubsystem::SetMissionPhase(FName MissionId, int32 Phase)
{
	if (!Profile || MissionId.IsNone()) return;

	Phase = FMath::Max(0, Phase);
	if (GetMissionPhase(MissionId) == Phase) return;

	Profile->MissionPhases.Add(MissionId, Phase);
	UE_LOG(LogEgnisSave, Log, TEXT("[Profile] Mision %s -> fase %d"), *MissionId.ToString(), Phase);
	SaveProfile();
}

int32 UEgnisProfileSubsystem::GetMissionPhase(FName MissionId) const
{
	if (!Profile) return 0;
	const int32* Phase = Profile->MissionPhases.Find(MissionId);
	return Phase ? *Phase : 0;
}

//Runs

void UEgnisProfileSubsystem::RegisterRunStarted()
{
	if (!Profile) return;
	Profile->RunsStarted++;
	SaveProfile();
}

void UEgnisProfileSubsystem::RegisterRunEnded(bool bWon)
{
	if (!Profile) return;
	if (bWon)
	{
		Profile->RunsWon++;
	}
	SaveProfile();
}

//Debug

FString UEgnisProfileSubsystem::DescribeProfile() const
{
	if (!Profile) return TEXT("(sin perfil)");

	auto JoinNames = [](const TSet<FName>& Names)
	{
		TArray<FString> Parts;
		for (const FName& Name : Names) Parts.Add(Name.ToString());
		return Parts.Num() > 0 ? FString::Join(Parts, TEXT(", ")) : FString(TEXT("(ninguno)"));
	};

	TArray<FString> Missions;
	for (const TPair<FName, int32>& Pair : Profile->MissionPhases)
	{
		Missions.Add(FString::Printf(TEXT("%s=%d"), *Pair.Key.ToString(), Pair.Value));
	}

	// Descripción del perfil
	return FString::Printf(
		TEXT("  Version: %d\n  Heroes desbloqueados: %s\n  Encuentros vistos: %s\n  Misiones: %s\n  Runs empezadas/ganadas: %d/%d"),
		Profile->SaveVersion,
		*JoinNames(Profile->UnlockedHeroes),
		*JoinNames(Profile->SeenEncounters),
		Missions.Num() > 0 ? *FString::Join(Missions, TEXT(", ")) : TEXT("(ninguna)"),
		Profile->RunsStarted, Profile->RunsWon);
}


void UEgnisProfileSubsystem::LogProfile() const
{
	TArray<FString> Lines;
	DescribeProfile().ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		UE_LOG(LogEgnisSave, Log, TEXT("[Profile] %s"), *Line);
	}
}

// ===== Comandos de consola de prueba =====
//   Egnis.Profile.Dump                     -> muestra todo el perfil
//   Egnis.Profile.Unlock <Heroe>           -> desbloquea un héroe para siempre
//   Egnis.Profile.Seen <Encuentro>         -> marca un encuentro como visto
//   Egnis.Profile.Mission <Mision> <Fase>  -> pone una misión en una fase
//   Egnis.Profile.Save                     -> fuerza el guardado
//   Egnis.Profile.Reset                    -> borra todo el progreso permanente


//Esto es solo para debug, no se usan en el juego final
//Se pueden usar para probar el guardado de perfil y desbloquear heroes o misiones sin tener que jugar la run completa
//TODO: Modificar  para eventos, misiones, etc. según se vayan añadiendo
//Funcionan en cualquier mapa mientras el juego está en Play

namespace
{
	UEgnisProfileSubsystem* GetProfileSubsystem(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UEgnisProfileSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UEgnisProfileSubsystem>() : nullptr;
		if (!Subsystem)
		{
			UE_LOG(LogEgnisSave, Warning, TEXT("[Profile] Comando ignorado: el juego tiene que estar en Play."));
		}
		return Subsystem;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdProfileDump(
		TEXT("Egnis.Profile.Dump"),
		TEXT("Muestra el contenido del perfil."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UEgnisProfileSubsystem* Profile = GetProfileSubsystem(World))
			{
				Profile->LogProfile();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdProfileUnlock(
		TEXT("Egnis.Profile.Unlock"),
		TEXT("Desbloquea un heroe para siempre. Uso: Egnis.Profile.Unlock DA_Tothum"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogEgnisSave, Warning, TEXT("[Profile] Uso: Egnis.Profile.Unlock <NombreHeroe>"));
				return;
			}
			if (UEgnisProfileSubsystem* Profile = GetProfileSubsystem(World))
			{
				Profile->UnlockHeroForever(FName(*Args[0]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdProfileSeen(
		TEXT("Egnis.Profile.Seen"),
		TEXT("Marca un encuentro como visto. Uso: Egnis.Profile.Seen EV_Ru"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogEgnisSave, Warning, TEXT("[Profile] Uso: Egnis.Profile.Seen <NombreEncuentro>"));
				return;
			}
			if (UEgnisProfileSubsystem* Profile = GetProfileSubsystem(World))
			{
				Profile->MarkEncounterSeen(FName(*Args[0]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdProfileMission(
		TEXT("Egnis.Profile.Mission"),
		TEXT("Pone una mision en una fase. Uso: Egnis.Profile.Mission MS_MitoGhol 2"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() < 2)
			{
				UE_LOG(LogEgnisSave, Warning, TEXT("[Profile] Uso: Egnis.Profile.Mission <NombreMision> <Fase>"));
				return;
			}
			if (UEgnisProfileSubsystem* Profile = GetProfileSubsystem(World))
			{
				Profile->SetMissionPhase(FName(*Args[0]), FCString::Atoi(*Args[1]));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdProfileSave(
		TEXT("Egnis.Profile.Save"),
		TEXT("Fuerza el guardado del perfil en disco."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UEgnisProfileSubsystem* Profile = GetProfileSubsystem(World))
			{
				Profile->SaveProfile();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdProfileReset(
		TEXT("Egnis.Profile.Reset"),
		TEXT("Borra todo el progreso permanente."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UEgnisProfileSubsystem* Profile = GetProfileSubsystem(World))
			{
				Profile->ResetProfile();
			}
		}));
}

// PRUEBAS
// B1. Perfil nuevo
//   1. Sin Play, borrar Profile.sav
//   2. Play y escribir Egnis.Profile.Dump
//   -> Al arrancar sale "Perfil nuevo creado"; en el Dump todo "(ninguno)" y "0/0".
//
// B2. Lo guardado sobrevive a cerrar el juego
//   1. En Play, escribir uno por uno:
//        Egnis.Profile.Unlock DA_Tothüm
//        Egnis.Profile.Seen EV_Ru
//        Egnis.Profile.Mission MS_MitoGhol 2
//      -> Cada uno saca su mensaje y "Guardado en disco"
//   2. Parar el Play y volver a darle
//      -> Al arrancar, "Perfil cargado" con DA_Tothüm, EV_Ru y MS_MitoGhol=2.
//   3. Repetir Egnis.Profile.Unlock DA_Tothüm
//      -> NO sale "Guardado en disco" (ya estaba desbloqueado)
//
// B3. Archivo roto
//   1. Sin Play, abrir Profile.sav con el Bloc de notas, escribir cualquier cosa y guardar
//   2. Play
//   -> Sale "El archivo existe pero no se puede leer..." y luego "Perfil nuevo creado"
//      El juego no se cuelga
//
// B4. Reiniciar
//   1. En Play: Egnis.Profile.Seen EV_Ru y después Egnis.Profile.Reset
//   2. Parar el Play y volver a darle
//   -> Todo vacío
//
// EXTRA. Sin estar en Play, escribir Egnis.Profile.Dump en la barra Cmd del editor
//   -> Sale "Comando ignorado: el juego tiene que estar en Play