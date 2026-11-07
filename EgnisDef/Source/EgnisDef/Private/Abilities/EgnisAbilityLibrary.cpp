#include "Abilities/EgnisAbilityLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/CharacterBase.h"
#include "EngineUtils.h"
#include "Cards/DeckManager.h"
#include "Characters/Ally.h"
#include "Characters/HeroDataAsset.h"
#include "CoreAndSystems/BattleManager.h"
#include "CoreAndSystems/BattleSceneManager.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/GameManager.h"

void UEgnisAbilityLibrary::SendEvent(AActor* Receiver, FGameplayTag EventTag, AActor* Instigator, AActor* Target,
	float Magnitude, const UObject* OptionalObject, const FGameplayTagContainer& InstigatorTags)
{
	if (!IsValid(Receiver) || !EventTag.IsValid() || !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Receiver))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Instigator;
	Payload.Target = Target;
	Payload.EventMagnitude = Magnitude;
	Payload.OptionalObject = OptionalObject;
	Payload.InstigatorTags = InstigatorTags;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Receiver, EventTag, Payload);
}

void UEgnisAbilityLibrary::SendEventToAllCharacters(const UObject* WorldContextObject, FGameplayTag EventTag, AActor* Instigator,
	AActor* Target, float Magnitude, const AActor* Excluded)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	TArray<ACharacterBase*> Characters;
	for (TActorIterator<ACharacterBase> It(World); It; ++It)
	{
		if (*It != Excluded)
		{
			Characters.Add(*It);
		}
	}

	for (ACharacterBase* Character : Characters)
	{
		SendEvent(Character, EventTag, Instigator, Target, Magnitude, nullptr, FGameplayTagContainer());
	}
}

bool UEgnisAbilityLibrary::HasRule(const AActor* Actor, FGameplayTag RuleTag)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
	return ASC && RuleTag.IsValid() && ASC->HasMatchingGameplayTag(RuleTag);
}

UDeckManager* UEgnisAbilityLibrary::GetDeckManager(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const AGameManager* GM = World ? Cast<AGameManager>(World->GetAuthGameMode()) : nullptr;
	const UBattleSceneManager* BSM = GM ? GM->GetBattleSceneManager() : nullptr;
	return BSM ? BSM->GetDeckManager() : nullptr;
}

UBaseCard* UEgnisAbilityLibrary::GetCardInPlay(const UObject* WorldContextObject)
{
	const UDeckManager* Deck = GetDeckManager(WorldContextObject);
	return Deck ? Deck->GetCardInPlay() : nullptr;
}

FText UEgnisAbilityLibrary::GetEnergyText(const AAlly* Ally)
{
	if (!Ally)
	{
		return FText::GetEmpty();
	}

	const FText Energy = FText::AsNumber(Ally->GetCurrentEnergy());
	const int32 Reserve = Ally->GetSecondWindEnergy();
	return Reserve > 0 ? FText::FormatOrdered(INVTEXT("{0}+{1}"), Energy, FText::AsNumber(Reserve)) : Energy;
}

bool UEgnisAbilityLibrary::AreEnemies(AActor* A, AActor* B)
{
	ACharacterBase* CharacterA = Cast<ACharacterBase>(A);
	ACharacterBase* CharacterB = Cast<ACharacterBase>(B);
	return IsValid(CharacterA) && IsValid(CharacterB) && CharacterA->GetTeam() != CharacterB->GetTeam();
}

void UEgnisAbilityLibrary::GrantShield(ACharacterBase* Source, ACharacterBase* Target, TSubclassOf<UGameplayEffect> ShieldEffect, float Amount)
{
	UAbilitySystemComponent* ASC = IsValid(Target) ? Target->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !ShieldEffect || Amount <= 0.f)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	if (Source)
	{
		Context.AddInstigator(Source, Source);
	}

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ShieldEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, Amount);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UEgnisAbilityLibrary::DealDamage(ACharacterBase* Source, ACharacterBase* Target, TSubclassOf<UGameplayEffect> DamageEffect, float Amount)
{
	UAbilitySystemComponent* ASC = IsValid(Target) ? Target->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !DamageEffect || Amount <= 0.f)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	if (Source)
	{
		Context.AddInstigator(Source, Source);
	}

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -Amount);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

AAlly* UEgnisAbilityLibrary::SpawnSummon(ACharacterBase* SummonOwner, TSubclassOf<AAlly> SummonClass, UHeroDataAsset* SummonData, FTileCoord Tile, float Health)
{
	if (!IsValid(SummonOwner) || !SummonOwner->Board || !SummonClass || !SummonData || SummonOwner->Board->IsTileOccupied(Tile))
	{
		return nullptr;
	}

	ABoard* Board = SummonOwner->Board;
	UWorld* World = SummonOwner->GetWorld();
	const FTransform SpawnTransform(FRotator::ZeroRotator, Board->TileToWorldCenter(Tile));
	AAlly* Summon = World->SpawnActorDeferred<AAlly>(SummonClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Summon)
	{
		return nullptr;
	}

	Summon->Board = Board;
	Summon->CurrentTile = Tile;
	Summon->HeroAsset = SummonData;
	Summon->bIsSummon = true;
	Summon->SummonOwner = SummonOwner;
	Summon->HealthOverride = FMath::Max(1.f, Health);
	Summon->FinishSpawning(FTransform(FRotator::ZeroRotator, Summon->GetStandLocationForTile(Tile)));

	const AGameManager* GM = Cast<AGameManager>(World->GetAuthGameMode());
	const UBattleSceneManager* BSM = GM ? GM->GetBattleSceneManager() : nullptr;
	if (UBattleManager* BM = BSM ? BSM->GetBattleManager() : nullptr)
	{
		BM->RegisterUnit(Summon);
	}

	SendEvent(SummonOwner, TAG_Event_Summon_Created, SummonOwner, Summon, 0.f, nullptr, FGameplayTagContainer());
	return Summon;
}

TArray<ACharacterBase*> UEgnisAbilityLibrary::GetCharactersAround(ABoard* Board, FTileCoord Center, int32 Radius)
{
	TArray<ACharacterBase*> Result;
	if (!Board)
	{
		return Result;
	}

	TArray<FTileCoord> Tiles;
	Board->GetTilesInRange(Center, Radius, Tiles);
	for (const FTileCoord& Tile : Tiles)
	{
		if (ACharacterBase* Character = Cast<ACharacterBase>(Board->GetTileOccupant(Tile)))
		{
			Result.Add(Character);
		}
	}
	return Result;
}

TArray<ACharacterBase*> UEgnisAbilityLibrary::GetEnemiesBesideTarget(ACharacterBase* Source, ACharacterBase* Target)
{
	TArray<ACharacterBase*> Result;
	if (!IsValid(Source) || !IsValid(Target) || !Source->Board)
	{
		return Result;
	}

	const int32 DX = Target->CurrentTile.X - Source->CurrentTile.X;
	const int32 DY = Target->CurrentTile.Y - Source->CurrentTile.Y;
	const int32 AbsX = FMath::Abs(DX);
	const int32 AbsY = FMath::Abs(DY);
	const int32 DirX = AbsX >= AbsY ? FMath::Sign(DX) : 0;
	const int32 DirY = AbsY >= AbsX ? FMath::Sign(DY) : 0;

	for (const int32 Side : { -1, 1 })
	{
		const FTileCoord Tile(Target->CurrentTile.X - DirY * Side, Target->CurrentTile.Y + DirX * Side);
		ACharacterBase* Other = Cast<ACharacterBase>(Source->Board->GetTileOccupant(Tile));
		if (AreEnemies(Source, Other))
		{
			Result.Add(Other);
		}
	}
	return Result;
}

TArray<ACharacterBase*> UEgnisAbilityLibrary::GetSeedingTargets(ACharacterBase* Caster, bool bAllies)
{
	TArray<ACharacterBase*> Result;
	if (!IsValid(Caster) || !Caster->Board)
	{
		return Result;
	}

	TArray<ACharacterBase*> Frontier = { Caster };
	TSet<ACharacterBase*> Visited = { Caster };
	while (Frontier.Num() > 0)
	{
		ACharacterBase* Current = Frontier.Pop();
		for (ACharacterBase* Neighbor : GetCharactersAround(Caster->Board, Current->CurrentTile, 1))
		{
			if (Visited.Contains(Neighbor) || (Neighbor->GetTeam() == Caster->GetTeam()) != bAllies)
			{
				continue;
			}

			Visited.Add(Neighbor);
			Result.Add(Neighbor);
			Frontier.Add(Neighbor);
		}
	}
	return Result;
}
