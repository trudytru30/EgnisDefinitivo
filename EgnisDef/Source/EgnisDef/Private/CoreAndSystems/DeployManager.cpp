#include "CoreAndSystems/DeployManager.h"
#include "Characters/Ally.h"
#include "Characters/HeroDataAsset.h"                
#include "CoreAndSystems/RunProgressionSubsystem.h"  

void UDeployManager::Initialize(TSubclassOf<AAlly> InBaseAllyClass)
{
	BaseAllyClass = InBaseAllyClass;
	DeployedHeroes.Empty();//lista nueva en cada despliegue
}

bool UDeployManager::TryDeployHeroAtLocation(FVector SpawnLocation, FRotator SpawnRotation)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AAlly* SpawnedAlly = GetWorld()->SpawnActor<AAlly>(BaseAllyClass, SpawnLocation, SpawnRotation, SpawnParams))
	{
		SpawnedAlly->InitializeFromData(SelectedHero);

		//apunta qué héroe es este personaje del tablero (para las heridas al acabar el combate)
		if (SelectedHero)
		{
			DeployedHeroes.Emplace(SpawnedAlly, URunProgressionSubsystem::GetHeroId(SelectedHero));
			UE_LOG(LogTemp, Log, TEXT("[DeployManager]: %s desplegado como %s"), *SpawnedAlly->GetName(), *SelectedHero->GetName());
		}

		SelectedHero = nullptr; 
        
		return true;
	}
	return false;
}
