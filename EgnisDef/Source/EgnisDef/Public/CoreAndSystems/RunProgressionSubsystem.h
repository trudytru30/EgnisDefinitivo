#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Characters/HeroRuntimeData.h"
#include "RunProgressionSubsystem.generated.h"

class UHeroDataAsset;

UCLASS()
class EGNISDEF_API URunProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Run|Deck")
	TArray<TSubclassOf<class UBaseCard>> PlayerCurrentDeck;

	UFUNCTION(BlueprintCallable, Category = "Run|Deck")
	TArray<TSubclassOf<UBaseCard>> GetPlayerCurrentDeck() const { return PlayerCurrentDeck; }
	// Se ejecuta al iniciar una nueva run para registrar todos los héroes posibles
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	void InitializeRunRoster(const TArray<UHeroDataAsset*>& AllHeroAssets);

	// Devuelve solo los héroes que están desbloqueados (para la UI de despliegue)
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	TArray<FHeroRuntimeData> GetUnlockedHeroes() const;

	// Desbloquear un personaje específico por su clave FName de la fila
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	void UnlockHero(FName HeroRowName);

	// Actualizar la vida de un personaje tras un combate
	UFUNCTION(BlueprintCallable, Category = "Run|Characters")
	void UpdateHeroHealth(FName HeroRowName, float NewHealth);

private:
	UPROPERTY()
	TMap<FName, FHeroRuntimeData> PlayerRoster;
};