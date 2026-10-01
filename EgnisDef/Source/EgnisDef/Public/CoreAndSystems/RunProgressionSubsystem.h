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
	// Se ejecuta al iniciar una nueva run para registrar todos los héroes posibles
	UFUNCTION(BlueprintCallable, Category = "Run Progression")
	void InitializeRunRoster(const TArray<UHeroDataAsset*>& AllHeroAssets);

	// Devuelve solo los héroes que están desbloqueados (para pintarlos en la UI de despliegue)
	UFUNCTION(BlueprintCallable, Category = "Run Progression")
	TArray<FHeroRuntimeData> GetUnlockedHeroes() const;

	// Desbloquear un personaje específico durante la partida
	UFUNCTION(BlueprintCallable, Category = "Run Progression")
	void UnlockHero(FName ExcelRowName);

	// Actualizar la vida de un personaje tras un combate
	UFUNCTION(BlueprintCallable, Category = "Run Progression")
	void UpdateHeroHealth(FName ExcelRowName, float NewHealth);

private:
	// Mapa interno para buscar de forma rápida por su clave única (el ExcelRowName)
	UPROPERTY()
	TMap<FName, FHeroRuntimeData> PlayerRoster;
};