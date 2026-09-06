#include "Cards/DeckManager.h"
#include "CoreAndSystems/AudioManager.h"
#include "Characters/Ally.h"

// Aniadir cartas al mazo
//Construye el mazo desde cero
void UDeckManager::GenerateDeck(const TArray<TSubclassOf<UBaseCard>>& SelectedCarts)
{
	if (SelectedCarts.Num() < InitialDeckSize)
	{
		UE_LOG(LogTemp, Error, TEXT("[DeckManager]: No hay suficientes cartas seleccionadas para generar el mazo"));
		return;
	}

	// Limpia mazo y aniadir las cartas
	ResetDeck();
	for (TSubclassOf<UBaseCard> Card : SelectedCarts)
	{
		if (!Card) continue;
		AddCardToDeck(NewObject<UBaseCard>(this, Card));
	}
}

// Inicializar mazo al principio de la batalla
//Se llama al empezar cada combate(coge todo el deck y lo convierte en drawPile
void UDeckManager::InitializeDeck()
{
	DrawPile = Deck;
	DiscardedPile.Empty();
	Hand.Empty();

	ShuffleDeck();
}

// Barajar cartas de forma aleatoria
void UDeckManager::ShuffleDeck()
{
	for (int32 i = 0; i < DrawPile.Num(); i++)
	{
		int32 RandomIndex = FMath::RandRange(0, DrawPile.Num() - 1);
		DrawPile.Swap(i, RandomIndex);
	}
}

// Robar cartas mediante habilidades
//Recorre drawPile e intercambia
void UDeckManager::DrawCard()
{
	if (DrawPile.Num() <= 0)
	{
		if (DiscardedPile.Num() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DeckManager]: Pila de descartes vacia"));
			return;
		}
		DrawPile = DiscardedPile;
		DiscardedPile.Empty();
		ShuffleDeck();
	}
	Hand.Add(DrawPile.Last());
	DrawPile.Pop();

	// Avisa a la carta recien robada de que ha entrado en mano,
	// por si alguno de sus efectos hace algo solo por estar disponible (sin jugarla). De momento
	// se llama sin dueño concreto (nullptr) — DeckManager representa el mazo del equipo, no esta
	// atado a una unidad. Revisar esta llamada si el primer efecto real que implemente el hook
	// necesita saber de quien es la mano.
	if (UBaseCard* DrawnCard = Hand.Last())
	{
		DrawnCard->NotifyHandEnter(nullptr);
	}

	if (UAudioManager* AM = GetWorld()->GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayDrawCardSound(0);
	OnHandChanged.Broadcast();
}

// Robar cartas (con habilidades)
void UDeckManager::DrawCardAmount(int32 Amount)
{
	for (int32 i = 0; i < Amount; i++)
	{
		DrawCard();
	}
}

// Descartar cartas de la mano a la pila de descarte
void UDeckManager::DiscardCardFromHand(UBaseCard* Card)
{
	if (Hand.Contains(Card))
	{
		// avisar de que la carta sale de la mano, antes de moverla.
		if (Card)
		{
			Card->NotifyHandLeave(nullptr);
		}

		Hand.Remove(Card);
		DiscardedPile.Add(Card);
		OnHandChanged.Broadcast();
	}
}

// Descartar cartas del mazo a la pila de descarte
void UDeckManager::DiscardCardFromDrawPile(UBaseCard* Card)
{
	if (DrawPile.Contains(Card))
	{
		DrawPile.Remove(Card);
		DiscardedPile.Add(Card);
	}
}

// Aniadir cartas al mazo permanentemente
void UDeckManager::AddCardToDeck(UBaseCard* Card)
{
	Deck.Add(Card);
}

// Eliminar una carta del mazo permanentemente
void UDeckManager::RemoveCardFromDeck(UBaseCard* Card)
{
	Deck.RemoveSingle(Card);
}

// Reiniciar mazo (al terminar una partida, no una batalla)
void UDeckManager::ResetDeck()
{
	Deck.Empty();
	DrawPile.Empty();
	DiscardedPile.Empty();
	Hand.Empty();
}

//grey-out por arquetipo
bool UDeckManager::IsCardArchetypeAvailable(const UBaseCard* Card, const TArray<ACharacterBase*>& CharactersInPlay) const
{
	if (!Card)
	{
		return false;
	}

	// Grey = arquetipo comun, siempre disponible, no depende de ningun personaje concreto.
	if (Card->GetColor() == EColorType::Grey)
	{
		return true;
	}

	for (const ACharacterBase* Character : CharactersInPlay)
	{
		const AAlly* CharAsAlly = Cast<const AAlly>(Character);
		if (CharAsAlly && CharAsAlly->GetArchetypeColor() == Card->GetColor())
		{
			return true;
		}
	}

	return false;
}

// ===== Getters =====
int32 UDeckManager::GetInitialHandSize()
{
	return InitialHandSize;
}

int32 UDeckManager::GetMaxHandSize()
{
	return MaxHandSize;
}

TArray<UBaseCard*> UDeckManager::GetHand()
{
	return Hand;
}

void UDeckManager::SetDeck(const TArray<UBaseCard*>& InDeck)
{
	ResetDeck();
	Deck = InDeck;
}