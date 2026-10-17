#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EgnisProfileSave.generated.h"

//lo que sobrevive entre runs y al cerrar el juego.
//solo guarda nombres (FName)
UCLASS()
class EGNISDEF_API UEgnisProfileSave : public USaveGame
{
	GENERATED_BODY()

public:
	//Version del savegame. Si se cambia, se puede hacer un upgrade de los datos en el constructor de la clase.
	static constexpr int32 CurrentSaveVersion = 1;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentSaveVersion;

	//Héroes que el jugador ha desbloqueado
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Profile")
	TSet<FName> UnlockedHeroes;

	//Encuentros que el jugador ha visto alguna vez como bosses , personajes , eventos 
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Profile")
	TSet<FName> SeenEncounters;

	//Misiones que el jugador ha completado, con su fase actual (0 = no empezada, 1 = empezada, 2 = completada)
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Profile")
	TMap<FName, int32> MissionPhases;

	
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Profile")
	int32 RunsStarted = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Profile")
	int32 RunsWon = 0;
};