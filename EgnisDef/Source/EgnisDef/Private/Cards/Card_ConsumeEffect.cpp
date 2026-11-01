#include "Cards/Card_ConsumeEffect.h"
#include "Cards/DeckManager.h"
#include "Algo/RandomShuffle.h"

void UCard_ConsumeEffect::Execute_None(UDeckManager* Deck)
{
	Super::Execute_None(Deck);

	if (!Deck) return;

	switch (Mode)
	{
	case EConsumeMode::PlayedCard:
		{
			UBaseCard* Card = Deck->GetCardInPlay();
			if (!Card)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Card_ConsumeEffect]: no hay carta en juego, no se consume nada"));
				return;
			}
			Deck->ConsumeCard(Card);
			break;
		}

	case EConsumeMode::ChosenFromHand:
		{
			// La eleccion es asincrona: el DeckManager llama a esto cuando el jugador confirma.
			// WeakDeck por si el mazo desaparece antes de que confirme.
			TWeakObjectPtr<UDeckManager> WeakDeck(Deck);
			Deck->RequestHandSelection(FMath::Max(1, Count), [WeakDeck](const TArray<UBaseCard*>& Chosen)
			{
				if (UDeckManager* StrongDeck = WeakDeck.Get())
				{
					for (UBaseCard* Card : Chosen)
					{
						StrongDeck->ConsumeCard(Card);
					}
				}
			});
			break;
		}

	case EConsumeMode::RandomFromHand:
		{
			UBaseCard* InPlay = Deck->GetCardInPlay();
			TArray<UBaseCard*> Candidates;
			for (UBaseCard* Card : Deck->GetHand())
			{
				if (Card && Card != InPlay)
				{
					Candidates.Add(Card);
				}
			}

			Algo::RandomShuffle(Candidates);
			const int32 NumToConsume = FMath::Min(FMath::Max(1, Count), Candidates.Num());
			for (int32 i = 0; i < NumToConsume; i++)
			{
				Deck->ConsumeCard(Candidates[i]);
			}
			break;
		}
	}
}