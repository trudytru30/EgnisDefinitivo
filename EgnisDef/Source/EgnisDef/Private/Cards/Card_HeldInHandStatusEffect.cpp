#include "Cards/Card_HeldInHandStatusEffect.h"
#include "Cards/DeckManager.h"
#include "Characters/Ally.h"
#include "Components/StatusEffectComponent.h"
#include "EngineUtils.h"

UCard_HeldInHandStatusEffect::UCard_HeldInHandStatusEffect()
{
	Target = ECardTarget::None;
	Status = EStatusType::Status_Madness;
}

void UCard_HeldInHandStatusEffect::Execute_OnTurnEndInHand(UDeckManager* Deck)
{
	Super::Execute_OnTurnEndInHand(Deck);

	UWorld* World = Deck ? Deck->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	TArray<AAlly*> Receivers;
	for (TActorIterator<AAlly> It(World); It; ++It)
	{
		AAlly* Ally = *It;
		if (IsValid(Ally) && Ally->StatusComp && (!Cult.IsValid() || Ally->HasCult(Cult)))
		{
			Receivers.Add(Ally);
		}
	}

	for (AAlly* Ally : Receivers)
	{
		UE_LOG(LogTemp, Log, TEXT("[HeldInHand] %s sigue en la mano al acabar el turno: %s recibe el estado %d"),
			*GetNameSafe(GetOuter()), *Ally->GetName(), static_cast<int32>(Status));
		Ally->StatusComp->ApplyStatus(Status, Turns, Ally, ApplicationTurn);
	}
}
