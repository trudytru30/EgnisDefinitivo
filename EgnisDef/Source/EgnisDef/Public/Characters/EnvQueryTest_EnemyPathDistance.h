#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_EnemyPathDistance.generated.h"

//Test EQS: casillas que hay que ANDAR hasta el heroe mas cercano, rodeando obstaculos y huecos.
//Se usa en vez del test "Distance" para acercarse (Inverse Linear: menos casillas = mejor)
UCLASS(meta = (DisplayName = "Distancia andando (Egnis)"))
class EGNISDEF_API UEnvQueryTest_EnemyPathDistance : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_EnemyPathDistance();

	//A quien se mide (por defecto todos los heroes vivos: cuenta el mas cercano andando)
	UPROPERTY(EditDefaultsOnly, Category = Distance)
	TSubclassOf<UEnvQueryContext> DistanceTo;

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};