#include "Cards/Card_DiscardEffect.h"
#include "Cards/DeckManager.h"
#include "Abilities/EgnisAbilityLibrary.h"

void UCard_DiscardEffect::Execute_Self(ACharacterBase* Source)
{
	Super::Execute_Self(Source);

	UDeckManager* Deck = UEgnisAbilityLibrary::GetDeckManager(Source);

	if (!Deck) return;
	
	TWeakObjectPtr<UDeckManager> WeakDeck(Deck);
	Deck->RequestHandSelection(FMath::Max(1, Count), [WeakDeck](const TArray<UBaseCard*>& Chosen)
	{
		if (UDeckManager* StrongDeck = WeakDeck.Get())
		{
			for (UBaseCard* Card : Chosen)
			{
				if (Card)
				{
					UE_LOG(LogTemp, Log, TEXT("[Card_DiscardEffect]: carta descartada: %s"), *Card->GetName());
					StrongDeck->DiscardCardFromHand(Card);
				}
			}
		}
	});
}