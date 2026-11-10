#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Characters/HeroRuntimeData.h"
#include "Progression/RunState.h"
#include "RunProgressionSubsystem.generated.h"

class UHeroDataAsset;
class UBaseCard;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunChanged);

//Mochila de la run: todo lo que el jugador lleva mientras dura una partida
//(héroes, heridas, reliquias, huesos, fogata, mazo)
//Vive en memoria: sobrevive a los cambios de mapa, pero no a cerrar el juego


UCLASS()
class EGNISDEF_API URunProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//PlayerCurrentDeck y GetPlayerCurrentDeck eliminados porque
	//ahora son datasset
	//Mirar mazo

	// Se ejecuta al iniciar una nueva run para registrar todos los héroes posibles
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	void InitializeRunRoster(const TArray<UHeroDataAsset*>& AllHeroAssets);

	// Devuelve solo los héroes que están desbloqueados (para la UI de despliegue)
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	TArray<FHeroRuntimeData> GetUnlockedHeroes() const;

	// Desbloquear un personaje específico por su clave FName de la fila
	//ahora es HeroId (nombre del DataAsset, ej. "DA_Tothüm")no la fila de clase
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	void UnlockHero(FName HeroId);

	// Actualizar la vida de un personaje tras un combate
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	void UpdateHeroHealth(FName HeroId, float NewHealth);

	// Sistema de guardado

	//suena cada vez que cambia algo de la mochila (para refrescar la UI)
	UPROPERTY(BlueprintAssignable, Category = "Run")
	FOnRunChanged OnRunChanged;

	//Ciclo run

	//vacía la mochila y empieza una run nueva con el equipo y el mazo iniciales
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartNewRun(const TArray<UHeroDataAsset*>& StartingHeroes, const TArray<UBaseCard*>& StartingDeck);

	//cierra la run. Los datos se quedan hasta la siguiente StartNewRun (pantalla de resultados)
	UFUNCTION(BlueprintCallable, Category = "Run")
	void EndRun(bool bWon);

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsRunActive() const { return bRunActive; }

	//Héroes

	//identificador de un héroe: el nombre de su DataAsset (ej DA_Tothüm)
	UFUNCTION(BlueprintPure, Category = "Run|Characters")
	static FName GetHeroId(const UHeroDataAsset* HeroAsset);

	//añade un héroe a la caravana a mitad de run (eventos, quests)
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	void AddHeroToRun(UHeroDataAsset* HeroAsset, bool bUnlocked = true);

	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	bool GetHeroState(FName HeroId, FHeroRuntimeData& OutHero) const;

	//Heridas

	//añade una herida aleatoria que el héroe no tenga
	//Devuelve cuál, o None si ya tiene el máximo
	UFUNCTION(BlueprintCallable, Category = "Run|Wounds")
	FName AddRandomWound(FName HeroId);

	//cura la herida más antigua. Devuelve false si no tenía ninguna
	UFUNCTION(BlueprintCallable, Category = "Run|Wounds")
	bool HealOneWound(FName HeroId);

	//Reliquias

	//mete una reliquia en la bolsa (sin equipar)
	UFUNCTION(BlueprintCallable, Category = "Run|Relics")
	void AddRelic(FName RelicId);

	//pasa una reliquia de la bolsa al héroe
	//Falla si no está en la bolsa o el héroe no tiene hueco
	UFUNCTION(BlueprintCallable, Category = "Run|Relics")
	bool EquipRelic(FName HeroId, FName RelicId);

	//devuelve la reliquia del héroe a la bolsa
	UFUNCTION(BlueprintCallable, Category = "Run|Relics")
	bool UnequipRelic(FName HeroId, FName RelicId);

	UFUNCTION(BlueprintPure, Category = "Run|Relics")
	TArray<FName> GetOwnedRelics() const { return OwnedRelics; }

	//Huesos

	UFUNCTION(BlueprintPure, Category = "Run|Bones")
	int32 GetBones() const { return Bones; }

	UFUNCTION(BlueprintCallable, Category = "Run|Bones")
	void AddBones(int32 Amount);

	//devuelve false (y no gasta nada) si no hay suficientes
	UFUNCTION(BlueprintCallable, Category = "Run|Bones")
	bool SpendBones(int32 Amount);

	//Fogata

	UFUNCTION(BlueprintPure, Category = "Run|Campfire")
	int32 GetCampfire() const { return Campfire; }

	//resta una llama (combate perdido). Si llega a 0 termina la run
	//devuelve true si la run ha terminado
	UFUNCTION(BlueprintCallable, Category = "Run|Campfire")
	bool LoseCampfire();

	//recupera llamas (campamento), sin pasar del máximo
	UFUNCTION(BlueprintCallable, Category = "Run|Campfire")
	void RestoreCampfire(int32 Amount = 1);

	//Mazo

	UFUNCTION(BlueprintPure, Category = "Run|Deck")
	TArray<FDeckEntry> GetDeck() const { return Deck; }

	//solo las cartas, en el formato que usa DeckManager::SetDeck
	UFUNCTION(BlueprintPure, Category = "Run|Deck")
	TArray<UBaseCard*> GetDeckCards() const;

	UFUNCTION(BlueprintCallable, Category = "Run|Deck")
	void AddCardToDeck(UBaseCard* Card);

	UFUNCTION(BlueprintCallable, Category = "Run|Deck")
	bool RemoveCardFromDeck(int32 DeckIndex);

	UFUNCTION(BlueprintCallable, Category = "Run|Deck")
	bool UpgradeCard(int32 DeckIndex);

	//Debug
	FString DescribeRun() const;
	void LogRun() const;

private:
	//HeroId (nombre del DataAsset del héroe)
	UPROPERTY()
	TMap<FName, FHeroRuntimeData> PlayerRoster;
	
	FHeroRuntimeData* FindHero(FName HeroId);
	void ResetRunData();
	void NotifyChanged();

	UPROPERTY()
	bool bRunActive = false;

	UPROPERTY()
	int32 Bones = 0;

	UPROPERTY()
	int32 Campfire = EgnisRun::MaxCampfire;

	UPROPERTY()
	TArray<FDeckEntry> Deck;

	//reliquias en la bolsa, sin equipar
	UPROPERTY()
	TArray<FName> OwnedRelics;
};