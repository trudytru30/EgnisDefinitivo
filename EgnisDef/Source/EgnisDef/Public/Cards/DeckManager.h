#pragma once

#include "CoreMinimal.h"
#include "Cards/BaseCard.h"
#include "DeckManager.generated.h"

class UAudioManager;
class UBaseCard;
class ACharacterBase;


UCLASS()
class EGNISDEF_API UDeckManager : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHandChanged);

	UPROPERTY(BlueprintAssignable)
	FOnHandChanged OnHandChanged;

	int32 GetDrawPileSize() const { return DrawPile.Num(); }
	
#pragma region Functions
	// Modificar las cartas del mazo elegidas por el player
	void GenerateDeck(const TArray<TSubclassOf<UBaseCard>>& SelectedCarts);
	void InitializeDeck();	// Generar mazo para la batalla
	void ResetDeck();	// Limpiar los mazo despues de cada batalla (Draw, Discarded y hand)
	void DrawCard();
	void DrawCardAmount(int32 Amount);
	void ShuffleDeck();
	
	void DiscardCardFromHand(UBaseCard* Card);
	void DiscardCardFromDrawPile(UBaseCard* Card);

	void AddCardToDeck(UBaseCard* Card);
	void RemoveCardFromDeck(UBaseCard* Card);

	UFUNCTION(BlueprintCallable, Category="Cards")
	void AddCardToHand(UBaseCard* Card);

	UFUNCTION(BlueprintPure, Category="Cards")
	TArray<UBaseCard*> GetDiscardPile() const { return DiscardedPile; }

	UFUNCTION(BlueprintCallable, Category="Cards")
	bool ReturnCardFromDiscardToHand(UBaseCard* Card);
	// Consumir: saca la carta de la mano o de las pilas y la guarda en ConsumedPile hasta que acabe el combate.
	// No toca Deck ni los assets, asi que en el siguiente combate la carta vuelve a estar.
	void ConsumeCard(UBaseCard* Card);
	
	UFUNCTION(BlueprintPure, Category="Cards")
	int32 GetConsumedPileSize() const { return ConsumedPile.Num(); }

	// Carta que se esta jugando ahora mismo. La pone UBaseCard::Execute y la leen efectos como Consumir.
	UBaseCard* GetCardInPlay() const { return CardInPlay; }
	void SetCardInPlay(UBaseCard* Card) { CardInPlay = Card; }
	
	// ========== Seleccion de cartas de la mano (para Consumir, y mas adelante Descartar y Guardar) ==========
	// Pide al jugador que marque exactamente Count cartas de la mano; OnConfirmed recibe las marcadas al confirmar.
	// La carta en juego no es elegible. Si hay Count o menos cartas elegibles se resuelve solo, sin pedir nada.
	// Si ya hay una seleccion activa, la nueva espera en cola.
	void RequestHandSelection(int32 Count, TFunction<void(const TArray<UBaseCard*>&)> OnConfirmed);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHandSelectionChanged);

	// Salta al empezar, marcar/desmarcar y terminar una seleccion. La UI se refresca con esto.
	UPROPERTY(BlueprintAssignable)
	FOnHandSelectionChanged OnHandSelectionChanged;

	UFUNCTION(BlueprintPure, Category="Cards|Seleccion")
	bool IsSelectingFromHand() const { return bSelecting; }

	// Cuantas cartas hay que marcar en la seleccion actual
	UFUNCTION(BlueprintPure, Category="Cards|Seleccion")
	int32 GetRequiredSelection() const { return RequiredSelection; }

	UFUNCTION(BlueprintPure, Category="Cards|Seleccion")
	TArray<UBaseCard*> GetSelectedCards() const { return SelectedCards; }

	// Para pintar la mano: si la carta se puede marcar ahora / si ya esta marcada
	UFUNCTION(BlueprintPure, Category="Cards|Seleccion")
	bool IsCardSelectable(const UBaseCard* Card) const;

	UFUNCTION(BlueprintPure, Category="Cards|Seleccion")
	bool IsCardSelected(const UBaseCard* Card) const;

	// El boton de confirmar solo se activa cuando estan marcadas exactamente las necesarias
	UFUNCTION(BlueprintPure, Category="Cards|Seleccion")
	bool CanConfirmHandSelection() const { return bSelecting && SelectedCards.Num() == RequiredSelection; }

	// Marca o desmarca. Devuelve false si no se puede.
	UFUNCTION(BlueprintCallable, Category="Cards|Seleccion")
	bool ToggleCardSelection(UBaseCard* Card);

	UFUNCTION(BlueprintCallable, Category="Cards|Seleccion")
	void ConfirmHandSelection();
	
	
	// Al empezar el combate: las cartas del mazo cuyo color no tiene ningun personaje desplegado pasan a Grey.
	// Solo afecta a las copias de este combate (SetDeck duplica las cartas), no al mazo de la run.
	// Llamar despues de SetDeck.
	void ConvertUnownedCardsToGrey(const TArray<ACharacterBase*>& CharactersInPlay);
	
	// Si hay algún personaje en CharactersInPlay cuyo arquetipo coincide con el
	// color de esta carta: Grey siempre cuenta como disponible.
	UFUNCTION(BlueprintCallable, Category="Cards")
	bool IsCardArchetypeAvailable(const UBaseCard* Card, const TArray<ACharacterBase*>& CharactersInPlay) const;
	
	// Getters/Setters
	int32 GetInitialHandSize();
	int32 GetMaxHandSize();
	TArray<UBaseCard*> GetHand();
	void SetDeck(const TArray<UBaseCard*>& InDeck);
#pragma endregion

private:

	void StartNextSelection();
	void ClearSelectionState();
	
#pragma region Properties
	UPROPERTY()
	TArray<UBaseCard*> TotalCards;

	UPROPERTY()
	TArray<UBaseCard*> Deck;

	UPROPERTY()
	TArray<UBaseCard*> DrawPile;

	UPROPERTY()
	TArray<UBaseCard*> DiscardedPile;

	UPROPERTY()
	TArray<UBaseCard*> Hand;
	
	UPROPERTY()
	TArray<UBaseCard*> ConsumedPile;

	UPROPERTY()
	UBaseCard* CardInPlay = nullptr;
	
	// Peticion de seleccion pendiente. La primera de SelectionQueue es la activa.
	struct FHandSelectionRequest
	{
		int32 Count = 0;
		TFunction<void(const TArray<UBaseCard*>&)> OnConfirmed;
		// Carta en juego al hacer la peticion; solo se compara el puntero, no es elegible
		UBaseCard* Excluded = nullptr;
	};
	TArray<FHandSelectionRequest> SelectionQueue;

	UPROPERTY()
	TArray<UBaseCard*> EligibleForSelection;

	UPROPERTY()
	TArray<UBaseCard*> SelectedCards;

	int32 RequiredSelection = 0;
	bool bSelecting = false;
#pragma endregion
	// TODO: InitialDeckSize, InitialHandSize y MaxHandSize se han movido a UEgnisCardSettings
	// (Project Settings > Egnis > Cartas) para que diseño pueda editarlos. Los getters las leen de alli.
};