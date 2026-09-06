#include "Cards/Card_AttackEffect.h"
#include "CoreAndSystems/AudioManager.h"

void UCard_AttackEffect::Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy)
{
	Super::Execute_Enemy(Self, Enemy);

	if (!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackEffect: Enemy is null"));
		return;
	}

	// Comprueba que la carta hace danio y que el objetivo es tipo characterBase
	if (DamageAmount >= 0 && Enemy->GetTeam() != Self->GetTeam())
	{
		Enemy->LossHealth(DamageAmount);
		if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayAttackSound(0);
	}
}