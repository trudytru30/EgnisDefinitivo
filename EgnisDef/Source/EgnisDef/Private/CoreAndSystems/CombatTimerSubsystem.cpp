#include "CoreAndSystems/CombatTimerSubsystem.h"

void UCombatTimerSubsystem::ModifyTime(float Seconds)
{
	if (!OnTimeModified.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatTimer]: se piden %.1f s pero no hay ningun temporizador escuchando."), Seconds);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CombatTimer]: %+.1f s al temporizador."), Seconds);
	OnTimeModified.Broadcast(Seconds);
}