#include "Cards/Card_KeepEffect.h"
#include "Cards/DeckManager.h"
#include "Abilities/EgnisAbilityLibrary.h"

void UCard_KeepEffect::Execute_Self(ACharacterBase* Source)
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
				StrongDeck->KeepCard(Card);
			}
		}
	});
}