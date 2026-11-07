#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_KeepEffect.generated.h"

class UDeckManager;

// Guardar: el jugador elige cartas de su mano y se apartan; vuelven a la mano al empezar su siguiente turno.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_KeepEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Keep", meta = (ClampMin = "1"))
	int32 Count = 1;

	virtual void Execute_None(UDeckManager* Deck) override;
};