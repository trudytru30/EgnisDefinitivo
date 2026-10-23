#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoreAndSystems/AudioTriggers.h"
#include "AudioManager.generated.h"

class UAudioDataAsset;
class UAudioComponent;
class UFMODEvent;
class AActor;

// Se lanza cada vez que ocurre un momento de audio (PlayTrigger), suene algo o no.
// Sirve para reaccionar desde Blueprint: poner parametros globales de FMOD, cambiar la musica, etc.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAudioTrigger, EEgnisAudioTrigger, Trigger, AActor*, Context);

UCLASS(BlueprintType)
class EGNISDEF_API UAudioManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// Asigna el DataAsset con todos los sonidos. Llamar una vez al inicio (ej: BeginPlay del Level Blueprint).
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetAudioData(UAudioDataAsset* Data);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	UAudioDataAsset* GetAudioData() const { return AudioData; }

#pragma region FMOD
	
	// ──────────────────────── FMOD ────────────────────────
	// Hace sonar el momento indicado. Orden de prioridad:
	//  1. OverrideEvent, si se pasa uno (p. ej. el sonido propio de un efecto de carta)
	//  2. El evento FMOD de ese momento en la tabla Trigger Events del AudioDataAsset
	//  3. El sonido antiguo (USoundBase) de ese momento, si lo tiene
	// Siempre lanza OnAudioTrigger. Context es el personaje/actor que provoca el sonido (puede ser nulo).
	UFUNCTION(BlueprintCallable, Category = "Audio|FMOD")
	void PlayTrigger(EEgnisAudioTrigger Trigger, UFMODEvent* OverrideEvent = nullptr, AActor* Context = nullptr);

	UPROPERTY(BlueprintAssignable, Category = "Audio|FMOD")
	FOnAudioTrigger OnAudioTrigger;
	
#pragma endregion
	
	// ── SFX ──────────────────────────────────────────────────────────────
	// El índice corresponde a la posición en el array del DataAsset (0 = primero).

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayAttackSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayHoverSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlaySelectCardSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayDrawCardSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayElevateSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayLandSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayDoruSpellSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayEgnisSpellSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayMenSpellSound(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayWersSpellSound(int32 Index);

	// Reproduce cualquier sonido directamente en 2D (sin necesidad de DataAsset).
	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlaySound(USoundBase* Sound);

	// ── MÚSICA ───────────────────────────────────────────────────────────

	// Reproduce una pista del array MusicTracks del DataAsset.
	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void PlayMusicByIndex(int32 Index);

	// Reproduce cualquier sonido como música de fondo (referencia directa).
	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void PlayMusic(USoundBase* Music);

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void StopMusic();

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void PauseMusic();

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void ResumeMusic();

	// Volumen de la música: 0.0 = silencio, 1.0 = volumen original.
	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "Audio|Music")
	bool IsMusicPlaying() const;

private:

	UPROPERTY()
	TObjectPtr<UAudioDataAsset> AudioData;

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	void PlaySoundFromArray(const TArray<TObjectPtr<USoundBase>>& Sounds, int32 Index, const TCHAR* CategoryName);
};