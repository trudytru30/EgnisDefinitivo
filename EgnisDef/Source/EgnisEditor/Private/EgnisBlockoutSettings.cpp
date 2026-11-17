#include "EgnisBlockoutSettings.h"

namespace
{
	TSoftClassPtr<AActor> EnemyBlueprint(const TCHAR* Name)
	{
		return TSoftClassPtr<AActor>(FSoftObjectPath(FString::Printf(TEXT("/Game/BPs/Gameplay/Characters/Enemies/%s.%s_C"), Name, Name)));
	}
}

UEgnisBlockoutSettings::UEgnisBlockoutSettings()
{
	CategoryName = TEXT("Egnis");
	SectionName = TEXT("Egnis Blockout");

	TileTypes.Add(TEXT("WALL"), ETileType::Obstacle);
	TileTypes.Add(TEXT("WATER"), ETileType::Water);
	TileTypes.Add(TEXT("BREAKABLE"), ETileType::BreakableObstacle);
	TileTypes.Add(TEXT("EXPLOSIVE"), ETileType::ExplosiveObstacle);
	TileTypes.Add(TEXT("NENUFAR"), ETileType::WaterPlatform);
	TileTypes.Add(TEXT("LIMITED"), ETileType::Limited);

	EnemyClasses.Add(TEXT("APERO"), EnemyBlueprint(TEXT("BP_Enemy_Apero")));
	EnemyClasses.Add(TEXT("UROU"), EnemyBlueprint(TEXT("BP_Enemy_Urou")));
	EnemyClasses.Add(TEXT("IKWOS"), EnemyBlueprint(TEXT("BP_Enemy_Ikwos")));
	EnemyClasses.Add(TEXT("KERVOSEI"), EnemyBlueprint(TEXT("BP_Enemy_Kervosi")));
	EnemyClasses.Add(TEXT("EWITO"), EnemyBlueprint(TEXT("BP_Enemy_Ewito")));

	IgnoredKeys = { TEXT("P1"), TEXT("P2"), TEXT("P3"), TEXT("P4") };

	LevelsFolder = TEXT("/Game/DataAssets/Levels/Area_{Zone}");
	AssetPrefix = TEXT("DA_L_");
}
