#include "CoreAndSystems/DeployManager.h"
#include "Characters/Ally.h"

void UDeployManager::Initialize(TSubclassOf<AAlly> InBaseAllyClass)
{
	BaseAllyClass = InBaseAllyClass;
}

bool UDeployManager::TryDeployHeroAtLocation(FVector SpawnLocation, FRotator SpawnRotation)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AAlly* SpawnedAlly = GetWorld()->SpawnActor<AAlly>(BaseAllyClass, SpawnLocation, SpawnRotation, SpawnParams))
	{
		SpawnedAlly->InitializeFromData(SelectedHero);

		SelectedHero = nullptr; 
        
		return true;
	}
	return false;
}
