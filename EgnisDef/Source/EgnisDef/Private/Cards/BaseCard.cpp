#include "Cards/BaseCard.h"

// Utilizar carta
void UBaseCard::Execute(UDeckManager* Deck, ACharacterBase* Self, ACharacterBase* TargetCharacter, FVector Location)
{
	for (UCardEffect* Effect : Effects)
	{
		switch (Effect->Target)
		{
		case ECardTarget::None:
			Effect->Execute_None(Deck);
			break;
		case ECardTarget::Ally:
			Effect->Execute_Ally(Self, TargetCharacter);
			break;
		case ECardTarget::Enemy:
			Effect->Execute_Enemy(Self, TargetCharacter);
			break;
		case ECardTarget::Self:
			Effect->Execute_Self(Self);
			break;
		case ECardTarget::Tile:
			Effect->Execute_Tile(Location);
			break;
		default:
			break;
		}
	}
}

//hooks de mano, delegan en cada efecto igual que Execute()
void UBaseCard::NotifyHandEnter(ACharacterBase* Owner)
{
	for (UCardEffect* Effect : Effects)
	{
		if (Effect)
		{
			Effect->Execute_OnHandEnter(Owner);
		}
	}
}

void UBaseCard::NotifyHandLeave(ACharacterBase* Owner)
{
	for (UCardEffect* Effect : Effects)
	{
		if (Effect)
		{
			Effect->Execute_OnHandLeave(Owner);
		}
	}
}

// ===== Getters =====
FText UBaseCard::GetCardName() const
{
	return CardName;
}

int32 UBaseCard::GetCost() const
{
	return Cost;
}

ECardType UBaseCard::GetCardType() const
{
	return CardType;
}

EColorType UBaseCard::GetColor() const
{
	return Color;
}

ECardTarget UBaseCard::GetTarget() const
{
	return Target;
}