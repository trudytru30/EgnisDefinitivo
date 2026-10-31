#include "Cards/Card_TimeEffect.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/CombatTimerSubsystem.h"
#include "Engine/GameInstance.h"
#include "CoreAndSystems/AudioManager.h"

UCard_TimeEffect::UCard_TimeEffect()
{
	Target = ECardTarget::Self;
}

void UCard_TimeEffect::Execute_Self(ACharacterBase* Source)
{
	Super::Execute_Self(Source);

	// La piedad solo la tienen los aliados
	const AAlly* Ally = Cast<AAlly>(Source);
	if (!Ally)
	{
		UE_LOG(LogTemp, Warning, TEXT("TimeEffect: quien juega la carta no es un aliado, no se toca el tiempo."));
		return;
	}

	// Cambiar temporizador: piedad del aliado x multiplicador de la carta
	const float Seconds = Ally->GetCharacterPiety() * PietyMultiplier;
	if (FMath::IsNearlyZero(Seconds))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] TimeEffect: piedad %.1f x %.1f da 0 segundos, no se hace nada."),
			*Ally->GetName(), Ally->GetCharacterPiety(), PietyMultiplier);
		return;
	}

	UGameInstance* GameInstance = Source->GetGameInstance();
	UCombatTimerSubsystem* Timer = GameInstance ? GameInstance->GetSubsystem<UCombatTimerSubsystem>() : nullptr;
	if (!Timer)
	{
		UE_LOG(LogTemp, Warning, TEXT("TimeEffect: no se encuentra CombatTimerSubsystem."));
		return;
	}

	Timer->ModifyTime(Seconds);
	if (UAudioManager* AM = GameInstance->GetSubsystem<UAudioManager>())
		AM->PlayTrigger(EEgnisAudioTrigger::CardTimeEffect, SoundOverride, Source);

	UE_LOG(LogTemp, Log, TEXT("TimeEffect: %s (piedad %.1f x %.1f) -> %+.1f s al temporizador."),
		*Ally->GetName(), Ally->GetCharacterPiety(), PietyMultiplier, Seconds);
}