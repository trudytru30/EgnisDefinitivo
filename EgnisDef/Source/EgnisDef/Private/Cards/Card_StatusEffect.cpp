#include "Cards/Card_StatusEffect.h"
#include "Components/StatusEffectComponent.h"

UCard_StatusEffect::UCard_StatusEffect()
{
	// Por defecto actua sobre un enemigo; para buffs se cambia a Ally o Self
	Target = ECardTarget::Enemy;
}

void UCard_StatusEffect::Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy)
{
	Super::Execute_Enemy(Self, Enemy);

	if (IsValid(Self) && IsValid(Enemy) && Self->GetTeam() == Enemy->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect: el objetivo Enemy es del mismo equipo"));
		return;
	}
	ApplyTo(Self, Enemy);
}

void UCard_StatusEffect::Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally)
{
	Super::Execute_Ally(Self, Ally);

	if (IsValid(Self) && IsValid(Ally) && Self->GetTeam() != Ally->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect: el objetivo Ally es de otro equipo"));
		return;
	}
	ApplyTo(Self, Ally);
}

void UCard_StatusEffect::Execute_Self(ACharacterBase* Source)
{
	Super::Execute_Self(Source);
	ApplyTo(Source, Source);
}

void UCard_StatusEffect::ApplyTo(ACharacterBase* Source, ACharacterBase* Receiver) const
{
	// IsValid y no solo != nullptr: otro efecto de la misma carta pudo matar ya al objetivo
	if (!IsValid(Source) || !IsValid(Receiver))
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect: Source o Receiver no son validos"));
		return;
	}

	if (UStatusEffectComponent* StatusComp = Receiver->StatusComp)
	{
		StatusComp->ApplyStatus(Status, Turns, Source);
	}
}