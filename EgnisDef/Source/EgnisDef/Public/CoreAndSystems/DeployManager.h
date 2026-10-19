#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoreAndSystems/Board.h"
#include "DeployManager.generated.h"

class UHeroDataAsset;
class AAlly;

UCLASS(BlueprintType)
class EGNISDEF_API UDeployManager : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeploymentChanged);

	void Initialize(TSubclassOf<AAlly> InBaseAllyClass, int32 InMaxDeployedHeroes);

	// Variable para guardar el héroe seleccionado desde la UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Director|Deployment")
	UHeroDataAsset* SelectedHero;

	UPROPERTY(EditDefaultsOnly, Category = "Scene Director|Deployment")
	TSubclassOf<AAlly> BaseAllyClass;

	//Se lanza cada vez que se coloca o se quita un héroe
	//La UI lo escucha para refrescar sus botones (en vez de guardar ella el estado)
	UPROPERTY(BlueprintAssignable, Category = "Scene Director|Deployment")
	FOnDeploymentChanged OnDeploymentChanged;

	//Coloca SelectedHero en la casilla. Falla si ya está desplegado, si se ha llegado al límite o si la casilla está ocupada
	UFUNCTION(BlueprintCallable, Category = "Scene Director|Deployment")
	bool TryDeployHeroAtTile(ABoard* Board, const FTileCoord& Tile);

	//Quita del tablero un héroe colocado en este despliegue (falla si el aliado no lo ha desplegado este manager)
	UFUNCTION(BlueprintCallable, Category = "Scene Director|Deployment")
	bool RemoveDeployedHero(AAlly* Ally);

	UFUNCTION(BlueprintPure, Category = "Scene Director|Deployment")
	bool IsHeroDeployed(const UHeroDataAsset* Hero) const;

	//Lo que usan los botones de la UI: no está ya en el tablero y queda hueco
	UFUNCTION(BlueprintPure, Category = "Scene Director|Deployment")
	bool CanDeployHero(const UHeroDataAsset* Hero) const;

	UFUNCTION(BlueprintPure, Category = "Scene Director|Deployment")
	bool HasDeployedHeroes() const { return DeployedHeroes.Num() > 0; }

	UFUNCTION(BlueprintPure, Category = "Scene Director|Deployment")
	int32 GetNumDeployedHeroes() const { return DeployedHeroes.Num(); }

	UFUNCTION(BlueprintPure, Category = "Scene Director|Deployment")
	int32 GetMaxDeployedHeroes() const { return MaxDeployedHeroes; }

	//Héroes desplegados en este combate: personaje del tablero + nombre del héroe (HeroId)
	//BattleSceneManager lo usa al acabar el combate: si el personaje ya no existe es que ese héroe ha muerto
	const TArray<TPair<TWeakObjectPtr<AAlly>, FName>>& GetDeployedHeroes() const { return DeployedHeroes; }

private:
	//Puntero débil: si el personaje muere y se destruye, se queda vacío solo
	TArray<TPair<TWeakObjectPtr<AAlly>, FName>> DeployedHeroes;

	//Lo configura BattleSceneManager en Initialize
	int32 MaxDeployedHeroes = 4;
};
