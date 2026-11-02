#include "Characters/EnvQueryContext_EnemyHeroes.h"
#include "Characters/CharacterBase.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "EngineUtils.h"

void UEnvQueryContext_EnemyHeroes::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	UWorld* World = QueryOwner ? QueryOwner->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	TArray<const AActor*> Heroes;
	for (TActorIterator<ACharacterBase> It(World); It; ++It)
	{
		if (It->GetTeam() == 0 && It->GetCurrentHealth() > 0.f)
		{
			Heroes.Add(*It);
		}
	}
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, Heroes);
}