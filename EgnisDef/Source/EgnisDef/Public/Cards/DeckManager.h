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
	// Getters
	int32 GetInitialHandSize();
	int32 GetMaxHandSize();
	TArray<UBaseCard*> GetHand();
	void SetDeck(const TArray<UBaseCard*>& InDeck);

	// nuevo reunion: ¿hay algún personaje en CharactersInPlay cuyo arquetipo
	// coincide con el color de esta carta? Grey (arquetipo común) siempre cuenta como disponible.
	// Pensado para que la UI (WBP_Card, cuando migremos BoardPlayerController/el HUD) decida si
	// mostrar la carta en gris. No se guarda la lista aquí: DeckManager no tiene referencia a las
	// unidades en juego — eso lo tiene BattleManager::GetCharactersOnField(), se le pasa cada vez.
	UFUNCTION(BlueprintCallable, Category="Cards")
	bool IsCardArchetypeAvailable(const UBaseCard* Card, const TArray<ACharacterBase*>& CharactersInPlay) const;
#pragma endregion

private:

#pragma region Properties
	UPROPERTY()
	TArray<UBaseCard*> TotalCards;

	UPROPERTY()
	TArray<TSubclassOf<UBaseCard>> SelectedCards;

	UPROPERTY()
	TArray<UBaseCard*> Deck;

	UPROPERTY()
	TArray<UBaseCard*> DrawPile;

	UPROPERTY()
	TArray<UBaseCard*> DiscardedPile;

	UPROPERTY()
	TArray<UBaseCard*> Hand;

	int32 InitialDeckSize = 20;
	int32 InitialHandSize = 3;
	int32 MaxHandSize = 9;
#pragma endregion
};