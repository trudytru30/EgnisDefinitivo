#include "Characters/EnvQueryTest_EnemyPathDistance.h"
#include "Characters/EnvQueryContext_EnemyHeroes.h"
#include "Characters/Enemy.h"
#include "Characters/CharacterBase.h"
#include "Characters/EnemyCombatLibrary.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "GameFramework/Controller.h"

UEnvQueryTest_EnemyPathDistance::UEnvQueryTest_EnemyPathDistance()
{
	Cost = EEnvTestCost::Medium;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	DistanceTo = UEnvQueryContext_EnemyHeroes::StaticClass();
	ScoringEquation = EEnvTestScoreEquation::InverseLinear; //menos casillas = mas puntos
}

void UEnvQueryTest_EnemyPathDistance::RunTest(FEnvQueryInstance& QueryInstance) const
{
	//El que pregunta puede ser el enemigo o su controlador
	UObject* QueryOwner = QueryInstance.Owner.Get();
	AEnemy* Enemy = Cast<AEnemy>(QueryOwner);
	if (!Enemy)
	{
		if (const AController* Controller = Cast<AController>(QueryOwner))
		{
			Enemy = Cast<AEnemy>(Controller->GetPawn());
		}
	}
	if (!Enemy || !Enemy->Board)
	{
		return;
	}
	const ABoard* Board = Enemy->Board;

	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	const float MinThreshold = FloatValueMin.GetValue();
	const float MaxThreshold = FloatValueMax.GetValue();

	TArray<AActor*> ContextActors;
	if (!QueryInstance.PrepareContext(DistanceTo, ContextActors))
	{
		return;
	}

	//Un mapa de distancias andando por cada heroe
	TArray<TMap<FTileCoord, int32>> Maps;
	TArray<FTileCoord> Goals;
	for (const AActor* Actor : ContextActors)
	{
		FTileCoord Goal;
		if (const ACharacterBase* Character = Cast<ACharacterBase>(Actor))
		{
			Goal = Character->CurrentTile;
		}
		else if (!Actor || !Board->WorldPointToTile(Actor->GetActorLocation(), Goal))
		{
			continue;
		}
		Goals.Add(Goal);
		UEnemyCombatLibrary::BuildPathDistanceMap(Board, Goal, Maps.AddDefaulted_GetRef());
	}
	if (Maps.Num() == 0)
	{
		return;
	}

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		FTileCoord Tile;
		if (!Board->WorldPointToTile(GetItemLocation(QueryInstance, It.GetIndex()), Tile))
		{
			It.ForceItemState(EEnvItemStatus::Failed);
			continue;
		}

		//Se queda con el heroe mas cercano andando
		int32 Best = MAX_int32;
		for (int32 i = 0; i < Maps.Num(); ++i)
		{
			const int32* Walk = Maps[i].Find(Tile);
			const int32 Value = Walk ? *Walk : 10000 + UEnemyCombatLibrary::TileDistance(Tile, Goals[i]);
			Best = FMath::Min(Best, Value);
		}
		It.SetScore(TestPurpose, FilterType, static_cast<float>(Best), MinThreshold, MaxThreshold);
	}
}

FText UEnvQueryTest_EnemyPathDistance::GetDescriptionTitle() const
{
	return FText::FromString(FString::Printf(TEXT("Distancia andando a %s (Egnis)"),
		*UEnvQueryTypes::DescribeContext(DistanceTo).ToString()));
}

FText UEnvQueryTest_EnemyPathDistance::GetDescriptionDetails() const
{
	return DescribeFloatTestParams();
}