#include "Cards/Card_DrawEffect.h"
#include "CoreAndSystems/AudioManager.h"
#include "Cards/DeckManager.h"
#include "Abilities/EgnisAbilityLibrary.h"

void UCard_DrawEffect::Execute_Self(ACharacterBase* Source)
{
	Super::Execute_Self(Source);

	UDeckManager* Deck = UEgnisAbilityLibrary::GetDeckManager(Source);

	if (!Deck) return;

	Deck->DrawCardAmount(DrawAmount);
}