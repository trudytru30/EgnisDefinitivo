#include "Cards/Card_AreaDamageEffect.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/AudioManager.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Environment/ObstacleBase.h"
#include "CoreAndSystems/CombatLibrary.h"

void UCard_AreaDamageEffect::Execute_Tile(ACharacterBase* Self, FVector Location)
{
	Super::Execute_Tile(Self, Location);

	if (!Self || !Self->Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("AreaDamageEffect: Self or Board is null"));
		return;
	}

	FTileCoord CenterTile;
	if (!Self->Board->WorldPointToTile(Location, CenterTile))
	{
		UE_LOG(LogTemp, Warning, TEXT("AreaDamageEffect: Location is outside the board"));
		return;
	}

	TArray<FTileCoord> AreaTiles;
	Self->Board->GetTilesInRange(CenterTile, AreaRadius, AreaTiles);
	AreaTiles.Add(CenterTile);

	int32 HitCount = 0;
	
	for (const FTileCoord& Tile : AreaTiles)
	{
		AActor* Occupant = Self->Board->GetTileOccupant(Tile);
		if (!Occupant)
		{
			continue;
		}

		if (ACharacterBase* Character = Cast<ACharacterBase>(Occupant))
		{
			if (Character->GetTeam() != Self->GetTeam())
			{
				UAbilitySystemComponent* TargetASC = Character->GetAbilitySystemComponent();
				if (TargetASC && DamageEffect)
				{
					FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
					FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);
					if (Spec.IsValid())
					{
						Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, 
							-UEgnisCombatLibrary::CalculateDamage(Self, Character, DamageAmount));
						TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
					}
				}
				HitCount++;
			}
			continue;
		}

		if (AObstacleBase* Obstacle = Cast<AObstacleBase>(Occupant))
		{
			Obstacle->ApplyDamage(DamageAmount);
			HitCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AreaDamage: centro (%d,%d), radio %d, %d objetivos golpeados"),
	CenterTile.X, CenterTile.Y, AreaRadius, HitCount);
	
	if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) 
		AM->PlayTrigger(EEgnisAudioTrigger::CardAreaAttack, SoundOverride, Self);
}