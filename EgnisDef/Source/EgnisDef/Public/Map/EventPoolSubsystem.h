#pragma once

#include "CoreMinimal.h"
#include "Map/MapTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventPoolSubsystem.generated.h"

class UEventPoolDataAsset;

/** Elige el evento de cada nodo Event a partir de la pool de su zona, bajando la probabilidad de los eventos que ya han salido en la run. */
UCLASS()
class EGNISDEF_API UEventPoolSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Devuelve el evento de este nodo (si ya se eligió, el mismo; si no, saca uno de la pool de su zona). */
	bool ResolveEventForNode(FName NodeID, EMapSection Section, FName& OutEventID);

	/** Olvida las veces que ha salido cada evento y los ya asignados a nodos. */
	UFUNCTION(BlueprintCallable, Category = "Map|Events")
	void ResetAll();

	void DumpToLog() const;

private:
	UFUNCTION()
	void HandleRunChanged();

	void LoadPools();
	const UEventPoolDataAsset* FindPool(EMapSection Section) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEventPoolDataAsset>> Pools;

	bool bPoolsLoaded = false;

	// "<zona>/<nodo>" -> evento asignado
	TMap<FString, FName> ResolvedByNode;

	// Veces que ha salido cada evento en esta run
	TMap<FName, int32> UseCount;
};