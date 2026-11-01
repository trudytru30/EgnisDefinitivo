#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_ConsumeEffect.generated.h"

class UDeckManager;

// Consumir: la carta que lo lleva se destruye durante lo que queda de combate (no vuelve a robarse ni se baraja).
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_ConsumeEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	virtual void Execute_None(UDeckManager* Deck) override;
};