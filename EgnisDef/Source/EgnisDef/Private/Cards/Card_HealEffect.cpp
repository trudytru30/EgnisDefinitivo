#include "Cards/Card_HealEffect.h"
#include "CoreAndSystems/AudioManager.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/Ally.h"

void UCard_HealEffect::Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally)
{
	Super::Execute_Ally(Self, Ally);

	// Valida parámetros
	if (!Self || !Ally)
	{
		UE_LOG(LogTemp, Warning, TEXT("HealEffect: Self or Ally is null"));
		return;
	}

	const float FinalAmount = GetFinalAmount(Self);
	if (FinalAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("HealEffect: la curacion final debe ser mayor que 0, pero es %.1f"), FinalAmount);
		return;
	}
	
	if (Self->GetTeam() != Ally->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("HealEffect: Cannot heal target on different team"));
		return;
	}

	// Aplicar curación
	UAbilitySystemComponent* TargetASC = Ally->GetAbilitySystemComponent();
	if (TargetASC && HealEffect)
	{
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(HealEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, FinalAmount);
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] HealEffect: falta ASC o HealEffect, no se aplica curación."),
			*Ally->GetName());
	}

	if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) 
		AM->PlayTrigger(EEgnisAudioTrigger::CardHeal, SoundOverride, Self);

	UE_LOG(LogTemp, Log, TEXT("HealEffect: %s healed %s for %.1f HP"), *Self->GetName(), *Ally->GetName(), FinalAmount);
}

float UCard_HealEffect::GetFinalAmount(const ACharacterBase* SelfChar) const
{
	if (PietyMultiplier > 0.f)
	{
		if (const AAlly* AllySelf = Cast<AAlly>(SelfChar))
		{
			return FMath::CeilToFloat(AllySelf->GetCharacterPiety() * PietyMultiplier);
		}
	}
	return HealAmount;
}