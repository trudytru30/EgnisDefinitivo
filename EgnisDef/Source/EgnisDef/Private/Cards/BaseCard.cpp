#include "Cards/BaseCard.h"
#include "Cards/EgnisCardSettings.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "Engine/Texture2D.h"
#include "Environment/ObstacleBase.h"
#include "CoreAndSystems/Board.h"
#include "Cards/DeckManager.h"
#include "Abilities/EgnisAbilityLibrary.h"
#include "Cards/Card_AttackEffect.h"
#include "Cards/Card_AreaDamageEffect.h"
#include "CoreAndSystems/AudioManager.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

// Utilizar carta
void UBaseCard::Execute(UDeckManager* Deck, ACharacterBase* Self, ACharacterBase* TargetCharacter, FVector Location)
{
	// Avisa al mazo de cual es la carta en juego (la usa el efecto Consumir)
	if (Deck)
	{
		Deck->SetCardInPlay(this);
	}
	
	// Sonido del culto: suena ademas del sonido base de cada efecto (las grises no tienen culto, no suenan)
	if (Self)
	{
		if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>())
			AM->PlayCultSound(Cult, Self);
	}
	
	// Energia al matar: se apunta quienes son enemigos de Self antes de los efectos, y al terminar se cuentan los
	// que ya no existen. Si uno muere (HandleDeath lo destruye) deja de ser valido.
	TArray<TWeakObjectPtr<ACharacterBase>> EnemiesBefore;
	if (EnergyOnKill > 0 && IsValid(Self))
	{
		for (TActorIterator<ACharacterBase> It(Self->GetWorld()); It; ++It)
		{
			ACharacterBase* Other = *It;
			if (IsValid(Other) && Other->GetTeam() != Self->GetTeam())
			{
				EnemiesBefore.Add(Other);
			}
		}
	}
	
	const int32 Repetitions = bPlaysTwice ? 2 : 1;
	for (int32 Rep = 0; Rep < Repetitions; ++Rep)
	{
		if (Rep > 0)
		{
			if (!IsValid(Self))
			{
				break;
			}
			if (TargetCharacter && !IsValid(TargetCharacter))
			{
				TargetCharacter = nullptr;
			}
		}
		
		for (UCardEffect* Effect : Effects)
		{
			switch (Effect->Target)
			{
			case ECardTarget::Ally:
				if (UEgnisAbilityLibrary::HasRule(Self, TAG_Rule_Targeting_Seeding))
				{
					for (ACharacterBase* Ally : UEgnisAbilityLibrary::GetSeedingTargets(Self, true))
					{
						if (IsValid(Ally))
						{
							Effect->Execute_Ally(Self, Ally);
						}
					}
					break;
				}
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
				if (UEgnisAbilityLibrary::HasRule(Self, TAG_Rule_Targeting_Seeding))
				{
					for (ACharacterBase* Enemy : UEgnisAbilityLibrary::GetSeedingTargets(Self, false))
					{
						if (IsValid(Enemy))
						{
							Effect->Execute_Enemy(Self, Enemy);
						}
					}
					break;
				}
				// Objetivo sin personaje: puede ser un obstaculo destructible, se busca por la casilla de Location
				if (!TargetCharacter && Self && Self->Board)
				{
					FTileCoord Tile;
					AObstacleBase* Obstacle = Self->Board->WorldPointToTile(Location, Tile)
						? Cast<AObstacleBase>(Self->Board->GetTileOccupant(Tile)) : nullptr;
					if (Obstacle && Obstacle->bDestructible)
					{
						Effect->Execute_Obstacle(Self, Obstacle);
						break;
					}
				}
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
		
	if (EnergyOnKill > 0 && IsValid(Self))
	{
		int32 Kills = 0;
		for (const TWeakObjectPtr<ACharacterBase>& Enemy : EnemiesBefore)
		{
			if (!Enemy.IsValid())
			{
				++Kills;
			}
		}

		if (Kills > 0)
		{
			if (AAlly* AllySelf = Cast<AAlly>(Self))
			{
				AllySelf->GainPointsIgnoringMax(Kills * EnergyOnKill);
				UE_LOG(LogTemp, Log, TEXT("[BaseCard] %s: %d bajas, %s recupera %d de energia (ahora %d/%d)"),
					*GetName(), Kills, *AllySelf->GetName(), Kills * EnergyOnKill,
					AllySelf->GetCurrentEnergy(), AllySelf->GetMaxEnergy());
			}
		}
	}
	
	if (Deck)
	{
		Deck->SetCardInPlay(nullptr);
	}
}

void UBaseCard::ExecuteAgainst(ACharacterBase* Self, ACharacterBase* Target)
{
	if (!Self || !Target)
	{
		return;
	}

	for (UCardEffect* Effect : Effects)
	{
		if (Effect && Effect->Target == ECardTarget::Enemy)
		{
			Effect->Execute_Enemy(Self, Target);
		}
	}
}

bool UBaseCard::HasDirectDamageEffect() const
{
	for (const UCardEffect* Effect : Effects)
	{
		if (Cast<UCard_AttackEffect>(Effect) || Cast<UCard_AreaDamageEffect>(Effect))
		{
			return true;
		}
	}
	return false;
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

bool UBaseCard::CanTargetCharacter(const ACharacterBase* Self, const ACharacterBase* Victim) const
{
	for (const UCardEffect* Effect : Effects)
	{
		if (Effect && (Effect->Target == ECardTarget::Ally || Effect->Target == ECardTarget::Enemy)
			&& !Effect->CanTargetCharacter(Self, Victim))
		{
			return false;
		}
	}
	return true;
}

bool UBaseCard::CanTargetObstacle() const
{
	for (const UCardEffect* Effect : Effects)
	{
		if (Effect && Effect->Target == ECardTarget::Enemy && Effect->CanTargetObstacle())
		{
			return true;
		}
	}
	return false;
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
	ECardTarget Result = ECardTarget::Self;
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