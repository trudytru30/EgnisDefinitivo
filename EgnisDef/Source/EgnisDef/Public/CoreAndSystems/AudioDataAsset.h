#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoreAndSystems/AudioTriggers.h"
#include "AudioDataAsset.generated.h"

class UFMODEvent;

UCLASS(BlueprintType)
class EGNISDEF_API UAudioDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	
	// ───────────────────────────── FMOD ─────────────────────────────
#pragma region FMOD
	
	// Tabla general: que evento FMOD suena en cada momento del juego (ver EEgnisAudioTrigger).
	// Una fila vacia = ese momento no suena (o usa el sonido antiguo de los arrays de abajo, si lo tiene).
	// Las filas se crean solas, solo hay que elegir el evento FMOD de cada una.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FMOD")
	TMap<EEgnisAudioTrigger, TObjectPtr<UFMODEvent>> TriggerEvents;

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

	// Anade a TriggerEvents una fila vacia por cada momento del enum que aun no la tenga
	void FillMissingTriggers();

#pragma endregion FMOD
	
	//  ────────────────── Sonidos antiguos (USoundBase por indice) ──────────────────
	// Siguen funcionando como respaldo mientras un momento no tenga evento FMOD asignado.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|Attack")
	TArray<TObjectPtr<USoundBase>> AttackSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|UI")
	TArray<TObjectPtr<USoundBase>> HoverSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|UI")
	TArray<TObjectPtr<USoundBase>> SelectCardSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|UI")
	TArray<TObjectPtr<USoundBase>> DrawCardSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|Movement")
	TArray<TObjectPtr<USoundBase>> ElevateSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|Movement")
	TArray<TObjectPtr<USoundBase>> LandSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|Spells")
	TArray<TObjectPtr<USoundBase>> DoruSpellSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|Spells")
	TArray<TObjectPtr<USoundBase>> EgnisSpellSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|Spells")
	TArray<TObjectPtr<USoundBase>> MenSpellSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX|Spells")
	TArray<TObjectPtr<USoundBase>> WersSpellSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	TArray<TObjectPtr<USoundBase>> MusicTracks;
};
