#include "Cards/Card_ShieldEffect.h"
#include "CoreAndSystems/AudioManager.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/Ally.h"

UCard_ShieldEffect::UCard_ShieldEffect()
{
	// Por defecto actua sobre un aliado (incluido uno mismo); en cartas de "solo yo" se cambia a Self
	Target = ECardTarget::Ally;
}

void UCard_ShieldEffect::Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally)
{
	Super::Execute_Ally(Self, Ally);
	ApplyShield(Self, Ally);
}

void UCard_ShieldEffect::Execute_Self(ACharacterBase* Source)
{
	Super::Execute_Self(Source);
	ApplyShield(Source, Source);
}

void UCard_ShieldEffect::ApplyShield(ACharacterBase* Source, ACharacterBase* Receiver) const
{
	if (!IsValid(Source) || !IsValid(Receiver))
	{
		UE_LOG(LogTemp, Warning, TEXT("ShieldEffect: Source o Receiver no son validos"));
		return;
	}

	const float FinalAmount = GetFinalAmount(Source);
	if (FinalAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShieldEffect: el escudo final debe ser mayor que 0, pero es %.1f"), FinalAmount);
		return;
	}

	if (Source->GetTeam() != Receiver->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("ShieldEffect: no se puede dar escudo a alguien de otro equipo"));
		return;
	}

	UAbilitySystemComponent* TargetASC = Receiver->GetAbilitySystemComponent();
	if (!TargetASC || !ShieldEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] ShieldEffect: falta ASC o ShieldEffect, no se aplica escudo."), *Receiver->GetName());
		return;
	}

	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(ShieldEffect, 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	// Mismo tag que usan daño y curacion: el GE es "Add" y solo cambia el atributo (Shield)
	Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, FinalAmount);
	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	if (UAudioManager* AM = Source->GetGameInstance()->GetSubsystem<UAudioManager>()) 
		AM->PlayTrigger(EEgnisAudioTrigger::CardShield, SoundOverride, Source);

	UE_LOG(LogTemp, Log, TEXT("ShieldEffect: %s dio %.1f de escudo a %s (escudo total %.1f)"),
		*Source->GetName(), FinalAmount, *Receiver->GetName(), Receiver->GetCurrentShield());
}

float UCard_ShieldEffect::GetFinalAmount(const ACharacterBase* SelfChar) const
{
	if (PietyMultiplier > 0.f)
	{
		if (const AAlly* AllySelf = Cast<AAlly>(SelfChar))
		{
			return FMath::CeilToFloat(AllySelf->GetCharacterPiety() * PietyMultiplier);
		}
	}
	return ShieldAmount;
}