#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EgnisProfileSubsystem.generated.h"

class UEgnisProfileSave;

//Al arrancar el juego, mira si existe el archivo del diario en el disco. Si existe, lo lee.
//Si no existe o está roto, abre un diario nuevo.
//Cada vez que se apunta algo (un desbloqueo, un encuentro, una misión), lo escribe en el disco en ese momento.
//Así nunca se pierde nada aunque el juego se cierre de golpe. 

//Se puede usar tanto en c++ como en blueprints
//En blueprints se puede usar para desbloquear héroes, marcar encuentros, avanzar misiones, etc.
UCLASS()
class EGNISDEF_API UEgnisProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	//Guardar/reiniciar
	UFUNCTION(BlueprintCallable, Category = "Profile")
	bool SaveProfile();

	//Borra todo el progreso permanente y lo guarda vacío
	UFUNCTION(BlueprintCallable, Category = "Profile")
	void ResetProfile();

	//Héroes desbloqueados permanentemente (los que se pueden usar en cualquier run)
	UFUNCTION(BlueprintCallable, Category = "Profile|Heroes")
	void UnlockHeroForever(FName HeroId);

	UFUNCTION(BlueprintPure, Category = "Profile|Heroes")
	bool IsHeroUnlockedForever(FName HeroId) const;

	//Encuentros vistos
	UFUNCTION(BlueprintCallable, Category = "Profile|Encounters")
	void MarkEncounterSeen(FName EncounterId);

	UFUNCTION(BlueprintPure, Category = "Profile|Encounters")
	bool HasSeenEncounter(FName EncounterId) const;

	//Misiones
	UFUNCTION(BlueprintCallable, Category = "Profile|Missions")
	void SetMissionPhase(FName MissionId, int32 Phase);

	//Devuelve 0 si la misión no se ha empezado nunca
	UFUNCTION(BlueprintPure, Category = "Profile|Missions")
	int32 GetMissionPhase(FName MissionId) const;

	//Estadisticas de runs (las llamará la mochila de la run)
	UFUNCTION(BlueprintCallable, Category = "Profile|Runs")
	void RegisterRunStarted();

	UFUNCTION(BlueprintCallable, Category = "Profile|Runs")
	void RegisterRunEnded(bool bWon);

	UFUNCTION(BlueprintPure, Category = "Profile")
	UEgnisProfileSave* GetProfile() const { return Profile; }

	//Texto con todo el contenido del diario
	FString DescribeProfile() const;//para debug
	
	//Escribe el contenido del diario en el log, una línea por dato
	void LogProfile() const;
	
private:
	void LoadOrCreateProfile();

	UPROPERTY()
	TObjectPtr<UEgnisProfileSave> Profile = nullptr;
};