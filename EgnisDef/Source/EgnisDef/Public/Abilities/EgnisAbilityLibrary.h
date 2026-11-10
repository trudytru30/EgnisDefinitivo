#pragma once

#include "CoreMinimal.h"
#include "CoreAndSystems/Board.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EgnisAbilityLibrary.generated.h"

class AAlly;
class ACharacterBase;
class UBaseCard;
class UDeckManager;
class UGameplayEffect;
class UHeroDataAsset;

UCLASS()
class EGNISDEF_API UEgnisAbilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Abilities|Events", meta = (AutoCreateRefTerm = "InstigatorTags", AdvancedDisplay = "Instigator,Target,Magnitude,OptionalObject,InstigatorTags"))
	static void SendEvent(AActor* Receiver, FGameplayTag EventTag, AActor* Instigator, AActor* Target,
		float Magnitude, const UObject* OptionalObject, const FGameplayTagContainer& InstigatorTags);

	UFUNCTION(BlueprintCallable, Category = "Abilities|Events", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "Instigator,Target,Magnitude,Excluded"))
	static void SendEventToAllCharacters(const UObject* WorldContextObject, FGameplayTag EventTag, AActor* Instigator = nullptr,
		AActor* Target = nullptr, float Magnitude = 0.f, const AActor* Excluded = nullptr);

	UFUNCTION(BlueprintPure, Category = "Abilities|Rules")
	static bool HasRule(const AActor* Actor, FGameplayTag RuleTag);

	UFUNCTION(BlueprintPure, Category = "Abilities|Cards", meta = (WorldContext = "WorldContextObject"))
	static UDeckManager* GetDeckManager(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Abilities|Cards", meta = (WorldContext = "WorldContextObject"))
	static UBaseCard* GetCardInPlay(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Abilities|UI")
	static FText GetEnergyText(const AAlly* Ally);

	UFUNCTION(BlueprintPure, Category = "Abilities|Combat")
	static bool AreEnemies(AActor* A, AActor* B);

	UFUNCTION(BlueprintCallable, Category = "Abilities|Combat")
	static void DealDamage(ACharacterBase* Source, ACharacterBase* Target, TSubclassOf<UGameplayEffect> DamageEffect, float Amount);

	UFUNCTION(BlueprintCallable, Category = "Abilities|Combat")
	static void GrantShield(ACharacterBase* Source, ACharacterBase* Target, TSubclassOf<UGameplayEffect> ShieldEffect, float Amount);

	UFUNCTION(BlueprintCallable, Category = "Abilities|Combat")
	static AAlly* SpawnSummon(ACharacterBase* SummonOwner, TSubclassOf<AAlly> SummonClass, UHeroDataAsset* SummonData, FTileCoord Tile, float Health);

	UFUNCTION(BlueprintCallable, Category = "Abilities|Board")
	static TArray<ACharacterBase*> GetCharactersAround(ABoard* Board, FTileCoord Center, int32 Radius);

	UFUNCTION(BlueprintCallable, Category = "Abilities|Board")
	static TArray<ACharacterBase*> GetEnemiesBesideTarget(ACharacterBase* Source, ACharacterBase* Target);

	UFUNCTION(BlueprintCallable, Category = "Abilities|Board")
	static TArray<ACharacterBase*> GetSeedingTargets(ACharacterBase* Caster, bool bAllies);
};
