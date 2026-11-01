#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_ConsumeEffect.generated.h"

class UDeckManager;

UENUM(BlueprintType)
enum class EConsumeMode : uint8
{
	PlayedCard		UMETA(DisplayName = "La propia carta"),
	ChosenFromHand	UMETA(DisplayName = "Elegida de la mano"),
	RandomFromHand	UMETA(DisplayName = "Al azar de la mano")
};

// Consumir: la carta (o cartas) se destruye durante lo que queda de combate (no vuelve a robarse ni se baraja).
// Target None: No necesita que el jugador elija nada al jugar la carta; si el modo es "elegida", la eleccion
// se pide despues, a traves de la seleccion de cartas del DeckManager.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_ConsumeEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	// Que carta se consume. La carta que se juega nunca es elegible en los modos de mano.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Consume")
	EConsumeMode Mode = EConsumeMode::PlayedCard;

	// Cuantas cartas se consumen. Si hay menos cartas elegibles, se consumen todas.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Consume",
		meta = (ClampMin = "1", EditCondition = "Mode != EConsumeMode::PlayedCard", EditConditionHides))
	int32 Count = 1;

	virtual void Execute_None(UDeckManager* Deck) override;
};