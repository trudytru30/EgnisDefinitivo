#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_EnemyHeroes.generated.h"

//Contexto EQS: todos los heroes vivos (para tests de distancia, etc.)
UCLASS(meta = (DisplayName = "Heroes (Egnis)"))
class EGNISDEF_API UEnvQueryContext_EnemyHeroes : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};