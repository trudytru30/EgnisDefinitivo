#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvQueryGenerator_EnemyReachableTiles.generated.h"

//Generador EQS: casillas a las que el enemigo puede llegar andando (mismo calculo que GetReachableTiles)
UCLASS(meta = (DisplayName = "Casillas alcanzables (Egnis)"))
class EGNISDEF_API UEnvQueryGenerator_EnemyReachableTiles : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_EnemyReachableTiles();

	//Incluir su propia casilla (quedarse quieto tambien es una opcion)
	UPROPERTY(EditDefaultsOnly, Category = Generator)
	bool bIncludeCurrentTile = true;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};