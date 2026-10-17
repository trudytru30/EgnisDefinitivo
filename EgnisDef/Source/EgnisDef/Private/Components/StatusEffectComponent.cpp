#include "Components/StatusEffectComponent.h"
#include "AbilitySystemComponent.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/EgnisGameplayTags.h"

namespace
{
	FGameplayTag GetTagForStatus(EStatusType Type)
	{
		switch (Type)
		{
		case EStatusType::Status_Burn:      return TAG_Status_Burn;
		case EStatusType::Status_Weakened:  return TAG_Status_Weakened;
		case EStatusType::Status_Exposed:   return TAG_Status_Exposed;
		case EStatusType::Status_Paralyzed: return TAG_Status_Paralyzed;
		case EStatusType::Status_Stunned:   return TAG_Status_Stunned;
		default:                            return FGameplayTag();
		}
	}
}

UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UStatusEffectComponent::GetOwnerASC() const
{
	const ACharacterBase* Character = Cast<ACharacterBase>(GetOwner());
	return Character ? Character->GetAbilitySystemComponent() : nullptr;
}

FActiveStatus* UStatusEffectComponent::FindStatus(EStatusType Type)
{
	return ActiveStatuses.FindByPredicate([Type](const FActiveStatus& S) { return S.Type == Type; });
}

const FActiveStatus* UStatusEffectComponent::FindStatus(EStatusType Type) const
{
	return ActiveStatuses.FindByPredicate([Type](const FActiveStatus& S) { return S.Type == Type; });
}

bool UStatusEffectComponent::HasStatus(EStatusType Type) const
{
	return FindStatus(Type) != nullptr;
}

int32 UStatusEffectComponent::GetTurnsLeft(EStatusType Type) const
{
	const FActiveStatus* Found = FindStatus(Type);
	return Found ? Found->TurnsLeft : 0;
}

bool UStatusEffectComponent::ApplyStatus(EStatusType Type, int32 Turns, ACharacterBase* Source)
{
	if (Turns <= 0 || !IsValid(GetOwner()))
	{
		return false;
	}

	if (FActiveStatus* Existing = FindStatus(Type))
	{
		// Ya activo: solo se refresca si la nueva duracion es mayor
		if (Turns > Existing->TurnsLeft)
		{
			Existing->TurnsLeft = Turns;
			Existing->bSkipNextDecrement = bOwnerTurnActive;
			Existing->Source = Source;
			OnStatusChanged.Broadcast();
		}
		return true;
	}

	FActiveStatus NewStatus;
	NewStatus.Type = Type;
	NewStatus.TurnsLeft = Turns;
	NewStatus.Source = Source;
	NewStatus.bSkipNextDecrement = bOwnerTurnActive;
	ActiveStatuses.Add(NewStatus);

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		ASC->AddLooseGameplayTag(GetTagForStatus(Type));
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Estado aplicado: %d (%d turnos)"), *GetOwner()->GetName(), (int32)Type, Turns);
	OnStatusChanged.Broadcast();
	return true;
}

void UStatusEffectComponent::RemoveStatus(EStatusType Type)
{
	const int32 Index = ActiveStatuses.IndexOfByPredicate([Type](const FActiveStatus& S) { return S.Type == Type; });
	if (Index == INDEX_NONE)
	{
		return;
	}

	ActiveStatuses.RemoveAt(Index);

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		ASC->RemoveLooseGameplayTag(GetTagForStatus(Type));
	}

	OnStatusChanged.Broadcast();
}

void UStatusEffectComponent::OnOwnerTurnStart()
{
	bOwnerTurnActive = true;
}

void UStatusEffectComponent::OnOwnerTurnEnd()
{
	bOwnerTurnActive = false;

	// Se recorre al reves porque los estados caducados se quitan del array durante el bucle
	bool bChanged = false;
	for (int32 i = ActiveStatuses.Num() - 1; i >= 0; --i)
	{
		FActiveStatus& Status = ActiveStatuses[i];

		if (Status.bSkipNextDecrement)
		{
			Status.bSkipNextDecrement = false;
			continue;
		}

		Status.TurnsLeft--;
		bChanged = true;

		if (Status.TurnsLeft <= 0)
		{
			if (UAbilitySystemComponent* ASC = GetOwnerASC())
			{
				ASC->RemoveLooseGameplayTag(GetTagForStatus(Status.Type));
			}
			ActiveStatuses.RemoveAt(i);
		}
	}

	if (bChanged)
	{
		OnStatusChanged.Broadcast();
	}
}