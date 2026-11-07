#include "Cards/Card_DiscardEffect.h"
#include "Cards/DeckManager.h"

void UCard_DiscardEffect::Execute_None(UDeckManager* Deck)
{
	Super::Execute_None(Deck);

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