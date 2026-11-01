#include "Cards/Card_ConsumeEffect.h"
#include "Cards/DeckManager.h"

void UCard_ConsumeEffect::Execute_None(UDeckManager* Deck)
{
	Super::Execute_None(Deck);

	if (!Deck) return;

	UBaseCard* Card = Deck->GetCardInPlay();
	if (!Card)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Card_ConsumeEffect]: no hay carta en juego, no se consume nada"));
		return;
	}

	Deck->ConsumeCard(Card);
}