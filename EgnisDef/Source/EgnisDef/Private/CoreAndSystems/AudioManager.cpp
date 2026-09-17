#include "CoreAndSystems/AudioManager.h"
#include "CoreAndSystems/AudioDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

void UAudioManager::SetAudioData(UAudioDataAsset* Data)
{
	AudioData = Data;
}

void UAudioManager::PlaySoundFromArray(const TArray<TObjectPtr<USoundBase>>& Sounds, int32 Index, const TCHAR* CategoryName)
{
	if (!Sounds.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: Índice %d fuera de rango en '%s' (tamaño: %d)"), Index, CategoryName, Sounds.Num());
		return;
	}

	USoundBase* Sound = Sounds[Index];
	if (!IsValid(Sound))
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: El sonido en índice %d de '%s' es null"), Index, CategoryName);
		return;
	}

	UGameplayStatics::PlaySound2D(GetGameInstance(), Sound);
}

// ── SFX ──────────────────────────────────────────────────────────────────────

void UAudioManager::PlayAttackSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->AttackSounds, Index, TEXT("AttackSounds"));
}

void UAudioManager::PlayHoverSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->HoverSounds, Index, TEXT("HoverSounds"));
}

void UAudioManager::PlaySelectCardSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->SelectCardSounds, Index, TEXT("SelectCardSounds"));
}

void UAudioManager::PlayDrawCardSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->DrawCardSounds, Index, TEXT("DrawCardSounds"));
}

void UAudioManager::PlayElevateSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->ElevateSounds, Index, TEXT("ElevateSounds"));
}

void UAudioManager::PlayLandSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->LandSounds, Index, TEXT("LandSounds"));
}

void UAudioManager::PlayDoruSpellSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->DoruSpellSounds, Index, TEXT("DoruSpellSounds"));
}

void UAudioManager::PlayEgnisSpellSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->EgnisSpellSounds, Index, TEXT("EgnisSpellSounds"));
}

void UAudioManager::PlayMenSpellSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->MenSpellSounds, Index, TEXT("MenSpellSounds"));
}

void UAudioManager::PlayWersSpellSound(int32 Index)
{
	if (!AudioData) return;
	PlaySoundFromArray(AudioData->WersSpellSounds, Index, TEXT("WersSpellSounds"));
}

void UAudioManager::PlaySound(USoundBase* Sound)
{
	if (!IsValid(Sound))
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: PlaySound recibió un sonido null"));
		return;
	}
	UGameplayStatics::PlaySound2D(GetGameInstance(), Sound);
}

// ── MÚSICA ───────────────────────────────────────────────────────────────────

void UAudioManager::PlayMusicByIndex(int32 Index)
{
	if (!AudioData) return;

	if (!AudioData->MusicTracks.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: Índice de música %d fuera de rango (tamaño: %d)"), Index, AudioData->MusicTracks.Num());
		return;
	}

	PlayMusic(AudioData->MusicTracks[Index]);
}

void UAudioManager::PlayMusic(USoundBase* Music)
{
	if (!IsValid(Music))
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: PlayMusic recibió un sonido null"));
		return;
	}

	StopMusic();

	MusicComponent = UGameplayStatics::SpawnSound2D(
		GetGameInstance(),
		Music,
		1.0f,    // VolumeMultiplier
		1.0f,    // PitchMultiplier
		0.0f,    // StartTime
		nullptr, // ConcurrencySettings
		true,    // bPersistAcrossLevelTransitions
		false    // bAutoDestroy — false para poder pausar/parar manualmente
	);
}

void UAudioManager::StopMusic()
{
	if (IsValid(MusicComponent))
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}
}

void UAudioManager::PauseMusic()
{
	if (IsValid(MusicComponent))
	{
		MusicComponent->SetPaused(true);
	}
}

void UAudioManager::ResumeMusic()
{
	if (IsValid(MusicComponent))
	{
		MusicComponent->SetPaused(false);
	}
}

void UAudioManager::SetMusicVolume(float Volume)
{
	if (IsValid(MusicComponent))
	{
		MusicComponent->SetVolumeMultiplier(FMath::Clamp(Volume, 0.0f, 1.0f));
	}
}

bool UAudioManager::IsMusicPlaying() const
{
	return IsValid(MusicComponent) && MusicComponent->IsPlaying();
}