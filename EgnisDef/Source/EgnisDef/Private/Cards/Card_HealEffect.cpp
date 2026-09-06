#include "Cards/Card_HealEffect.h"
#include "CoreAndSystems/AudioManager.h"

void UCard_HealEffect::Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally)
{
	Super::Execute_Ally(Self, Ally);

	// Validar parámetros
	if (!Self || !Ally)
	{
		UE_LOG(LogTemp, Warning, TEXT("HealEffect: Self or Ally is null"));
		return;
	}

	// Comprobar que la carta hace curacion y que el objetivo es del mismo equipo
	if (HealAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("HealEffect: HealAmount must be greater than 0, but is %.1f"), HealAmount);
		return;
	}

	if (Self->GetTeam() != Ally->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("HealEffect: Cannot heal target on different team"));
		return;
	}

	// Aplicar curación
	Ally->GainHealth(HealAmount);
	if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayDoruSpellSound(0);

	UE_LOG(LogTemp, Log, TEXT("HealEffect: %s healed %s for %.1f HP"), *Self->GetName(), *Ally->GetName(), HealAmount);
}