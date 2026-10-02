#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DeployManager.generated.h"

class UHeroDataAsset;
class AAlly;

UCLASS()
class EGNISDEF_API UDeployManager : public UObject
{
	GENERATED_BODY()

public:

	void Initialize(TSubclassOf<AAlly> InBaseAllyClass);

	// Variable para guardar el héroe seleccionado desde la UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Director|Deployment")
	UHeroDataAsset* SelectedHero;

	UPROPERTY(EditDefaultsOnly, Category = "Scene Director|Deployment")
	TSubclassOf<AAlly> BaseAllyClass;

	UFUNCTION(BlueprintCallable, Category = "Scene Director|Deployment")
	bool TryDeployHeroAtLocation(FVector SpawnLocation, FRotator SpawnRotation);
	
};