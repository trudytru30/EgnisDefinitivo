#include "Cards/Card_AttackEffect.h"
#include "CoreAndSystems/AudioManager.h"
#include "CoreAndSystems/EgnisGameplayTags.h"

void UCard_AttackEffect::Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy)
{
	Super::Execute_Enemy(Self, Enemy);

	if (!Self ||!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackEffect: Self or Enemy is null"));
		return;
	}

	// Comprueba que la carta hace daño y que el objetivo es tipo characterBase
	if (DamageAmount >= 0 && Enemy->GetTeam() != Self->GetTeam())
	{
		// El daño se aplica al ASC del objetivo, no al de quien ataca — un GameplayEffect
		// siempre modifica atributos de "a quién se le aplica", que aquí es Enemy.
		UAbilitySystemComponent* TargetASC = Enemy->GetAbilitySystemComponent();
		if (TargetASC && DamageEffect)
		{
			FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);
			if (Spec.IsValid())
			{
				// Negativo porque el Modifier del GE es "Add" — pasamos ya el signo de resta
				Spec.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -DamageAmount);
				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] AttackEffect: falta ASC o DamageEffect, no se aplica daño."), *Enemy->GetName());
		}

		if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayAttackSound(0);
	}
}