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

void UCombatTimerSubsystem::ReportTimeRemaining(float TimeRemaining, float TotalTime)
{
	OnTimeRemainingChanged.Broadcast(TimeRemaining, TotalTime);
}

void UCombatTimerSubsystem::PauseTimer(FName Reason)
{
	const bool bWasPaused = IsTimerPaused();
	PauseReasons.Add(Reason);
	if (!bWasPaused)
	{
		OnTimerPaused.Broadcast();
	}
}

void UCombatTimerSubsystem::ResumeTimer(FName Reason)
{
	if (PauseReasons.Remove(Reason) > 0 && !IsTimerPaused())
	{
		OnTimerResumed.Broadcast();
	}
}

void UCombatTimerSubsystem::ClearTimerPauses()
{
	if (IsTimerPaused())
	{
		PauseReasons.Empty();
		OnTimerResumed.Broadcast();
	}
}