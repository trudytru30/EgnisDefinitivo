#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_DiscardEffect.generated.h"

class UDeckManager;

UCLASS(EditInlineNew)
class EGNISDEF_API UCard_DiscardEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Discard", meta = (ClampMin = "1"))
	int32 Count = 1;

	virtual void Execute_None(UDeckManager* Deck) override;
};