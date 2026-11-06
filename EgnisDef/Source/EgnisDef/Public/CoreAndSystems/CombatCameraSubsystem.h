#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AttributeSet.h"
#include "TimerManager.h"
#include "CombatCameraSubsystem.generated.h"

class UAbilitySystemComponent;
class UCameraShakeBase;
class ACharacterBase;
class AAlly;

enum class ETimeSwayLevel : uint8
{
	None,
	Low,
	Med,
	High
};

// Efectos de camara del combate:
//  - balanceo (CameraTimeSway*) segun el tiempo restante del temporizador, continuo mientras dure ese nivel
//  - sacudida (CameraDamageShake) cada vez que un aliado pierde vida o escudo
// Los shakes y los umbrales se editan en Project Settings > Egnis > Camara (UEgnisCameraSettings).
UCLASS()
class EGNISDEF_API UCombatCameraSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// BattleManager lo llama al empezar el combate: deja todo a cero
	void BeginCombat();

	// ... y al terminarlo: para el balanceo y deja de escuchar el daño
	void EndCombat();

	// Empieza a escuchar el daño de este personaje (ignora lo que no sea AAlly). Se puede llamar varias veces con el mismo.
	void WatchAlly(ACharacterBase* Character);

private:
	// Lo dispara CombatTimerSubsystem cada vez que el WBP Combat Timer informa (cada 0.1 s)
	UFUNCTION()
	void HandleTimeRemaining(float TimeRemaining, float TotalTime);

	void SetSwayLevel(ETimeSwayLevel NewLevel);
	void PlaySway();   // arranca (o relanza) el shake del nivel actual
	void StopSway();
	void PlayDamageShake(const AAlly* Ally, float Amount);
	void UnwatchAll();
	TSubclassOf<UCameraShakeBase> GetSwayShake(ETimeSwayLevel Level) const;

	struct FWatchedAttribute
	{
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		FGameplayAttribute Attribute;
		FDelegateHandle Handle;
	};
	TArray<FWatchedAttribute> Watched;

	ETimeSwayLevel CurrentSway = ETimeSwayLevel::None;
	bool bCombatActive = false;
	FTimerHandle SwayRestartHandle;
};