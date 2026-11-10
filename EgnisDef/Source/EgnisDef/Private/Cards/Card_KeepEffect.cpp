#include "Cards/Card_KeepEffect.h"
#include "Cards/DeckManager.h"

void UCard_KeepEffect::Execute_None(UDeckManager* Deck)
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
				StrongDeck->KeepCard(Card);
			}
		}
	});
}