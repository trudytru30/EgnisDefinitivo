#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AudioDataAsset.generated.h"

UCLASS(BlueprintType)
class EGNISDEF_API UAudioDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

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
