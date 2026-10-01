#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "HeroDataAsset.generated.h"

class AAlly;
class UBaseCard;

UCLASS(BlueprintType)
class EGNISDEF_API UHeroDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UHeroDataAsset();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Excel Link", meta = (RowType = "HeroClassData"))
	FDataTableRowHandle HeroStatsRow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FText HeroName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	UTexture2D* SplashArt;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	USkeletalMesh* CharacterMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	UMaterialInterface* CharacterMaterial;
};