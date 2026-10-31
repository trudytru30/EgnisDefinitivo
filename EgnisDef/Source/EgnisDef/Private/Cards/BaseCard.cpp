#include "Cards/BaseCard.h"
#include "Cards/EgnisCardSettings.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "Engine/Texture2D.h"

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
			// Solo se aplica si el objetivo elegido es del mismo equipo que quien juega la carta
			if (!TargetCharacter || !Self || TargetCharacter->GetTeam() != Self->GetTeam())
			{
				UE_LOG(LogTemp, Warning, TEXT("[BaseCard]: %s tiene un efecto de aliado pero el objetivo elegido no "
								  "es un aliado, se omite"), *GetName());
				break;
			}
			Effect->Execute_Ally(Self, TargetCharacter);
			break;
		case ECardTarget::Enemy:
			// Solo se aplica si el objetivo elegido es del equipo contrario
			if (!TargetCharacter || !Self || TargetCharacter->GetTeam() == Self->GetTeam())
			{
				UE_LOG(LogTemp, Warning, TEXT("[BaseCard]: %s tiene un efecto de enemigo pero el objetivo elegido no "
								  "es un enemigo, se omite"), *GetName());
				break;
			}
			Effect->Execute_Enemy(Self, TargetCharacter);
			break;
		case ECardTarget::Self:
			Effect->Execute_Self(Self);
			break;
		case ECardTarget::Tile:
			//Effect->Execute_Tile(Location);
			Effect->Execute_Tile(Self,Location);
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

FText UBaseCard::GetDescription() const
{
	return Description;
}

void UBaseCard::SetCult(const FGameplayTag& NewCult)
{
	Cult = NewCult;
}

bool UBaseCard::CanBePlayedBy(const AAlly* Ally) const
{
	if (!Cult.IsValid())
	{
		return true;
	}
	return Ally && Ally->HasCult(Cult);
}

void UBaseCard::PostLoad()
{
	Super::PostLoad();

	if (!Cult.IsValid() && Color_DEPRECATED != EColorType::Grey)
	{
		Cult = EgnisCults::FromLegacyColor(Color_DEPRECATED);
		Color_DEPRECATED = EColorType::Grey;
	}
}

bool UBaseCard::CanTargetTile(const ACharacterBase* Self, const FTileCoord& Tile) const
{
	for (const UCardEffect* Effect : Effects)
	{
		if (Effect && Effect->Target == ECardTarget::Tile && !Effect->CanTargetTile(Self, Tile))
		{
			return false;
		}
	}
	return true;
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
	return EgnisCults::ToLegacyColor(Cult);
}

FLinearColor UBaseCard::GetCultColor() const
{
	return UEgnisCardSettings::GetCultColor(Cult);
}

ECardTarget UBaseCard::GetTarget() const
{
	// Objetivo que se pide al jugador al jugar la carta, deducido de los efectos (el campo Target de la carta ya no se
	// usa): el primer efecto que apunta a Ally, Enemy o Tile manda. Si no hay ninguno, Self si algun efecto es Self;
	// si no, None. Los efectos Self y None no necesitan que el jugador elija nada, se aplican solos con el resto.
	ECardTarget Result = ECardTarget::None;
	for (const UCardEffect* Effect : Effects)
	{
		if (!Effect)
		{
			continue;
		}

		switch (Effect->Target)
		{
		case ECardTarget::Ally:
		case ECardTarget::Enemy:
		case ECardTarget::Tile:
			return Effect->Target;
		case ECardTarget::Self:
			Result = ECardTarget::Self;
			break;
		default:
			break;
		}
	}
	return Result;
}

int32 UBaseCard::GetRange() const
{
	return Range;
}

UTexture2D* UBaseCard::GetCardImage() const
{
	return CardImage;
}