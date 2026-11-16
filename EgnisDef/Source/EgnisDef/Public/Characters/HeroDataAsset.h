#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Components/GridMovementComponent.h"
#include "HeroDataAsset.generated.h"

class AAlly;
class UBaseCard;
class UStaticMesh;
class UGameplayAbility;
class UFMODEvent;

UCLASS(BlueprintType)
class EGNISDEF_API UHeroDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UHeroDataAsset();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Excel Link", meta = (RowType = "HeroClassData"))
	FDataTableRowHandle HeroStatsRow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	EMovePattern MovePattern = EMovePattern::Straight_WBlock;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> UniqueAbility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FText HeroName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info")
	FText Alias;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	UTexture2D* SplashArt;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	UMaterialInterface* CharacterMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	UStaticMesh* CharacterModel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "0.01"))
	float ModelScale = 4.f;

	// Sonido de ataque propio del personaje: suena ademas del sonido base cuando ataca
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UFMODEvent> AttackSound;

	// Multiplicador de pitch de los sonidos de este personaje (1 = sin cambio, 1.2 = mas agudo)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.25", ClampMax = "4.0"))
	float PitchModifier = 1.f;
};