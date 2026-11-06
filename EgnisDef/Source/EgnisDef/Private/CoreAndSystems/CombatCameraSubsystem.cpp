#include "CoreAndSystems/CombatCameraSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/CombatTimerSubsystem.h"
#include "CoreAndSystems/EgnisCameraSettings.h"
#include "CoreAndSystems/HealthAttributeSet.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* SwayName(ETimeSwayLevel Level)
	{
		switch (Level)
		{
		case ETimeSwayLevel::Low:  return TEXT("Low");
		case ETimeSwayLevel::Med:  return TEXT("Med");
		case ETimeSwayLevel::High: return TEXT("High");
		default:                   return TEXT("None");
		}
	}
}

void UCombatCameraSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// El temporizador (WBP Combat Timer) avisa a traves del subsistema del GameInstance
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UCombatTimerSubsystem* Timer = GI->GetSubsystem<UCombatTimerSubsystem>())
		{
			Timer->OnTimeRemainingChanged.AddDynamic(this, &UCombatCameraSubsystem::HandleTimeRemaining);
		}
	}
}

void UCombatCameraSubsystem::Deinitialize()
{
	UnwatchAll();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SwayRestartHandle);

		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UCombatTimerSubsystem* Timer = GI->GetSubsystem<UCombatTimerSubsystem>())
			{
				Timer->OnTimeRemainingChanged.RemoveDynamic(this, &UCombatCameraSubsystem::HandleTimeRemaining);
			}
		}
	}

	Super::Deinitialize();
}

void UCombatCameraSubsystem::BeginCombat()
{
	EndCombat(); // limpia restos de un combate anterior
	bCombatActive = true;
}

void UCombatCameraSubsystem::EndCombat()
{
	StopSway();
	CurrentSway = ETimeSwayLevel::None;
	UnwatchAll();
	bCombatActive = false;
}

// ===== Balanceo segun el tiempo restante =====

void UCombatCameraSubsystem::HandleTimeRemaining(float TimeRemaining, float TotalTime)
{
	if (!bCombatActive || TotalTime <= 0.f)
	{
		return;
	}

	// Fraccion actual (puede pasar de 1 si se ha sumado tiempo extra): el nivel depende de ella,
	// asi que si vuelve a subir por encima de un umbral el balanceo baja de nivel o se apaga
	const float Ratio = TimeRemaining / TotalTime;
	const UEgnisCameraSettings* Settings = GetDefault<UEgnisCameraSettings>();

	ETimeSwayLevel NewLevel = ETimeSwayLevel::None;
	if (Ratio <= Settings->HighSwayThreshold)     NewLevel = ETimeSwayLevel::High;
	else if (Ratio <= Settings->MedSwayThreshold) NewLevel = ETimeSwayLevel::Med;
	else if (Ratio <= Settings->LowSwayThreshold) NewLevel = ETimeSwayLevel::Low;

	SetSwayLevel(NewLevel);
}

void UCombatCameraSubsystem::SetSwayLevel(ETimeSwayLevel NewLevel)
{
	if (NewLevel == CurrentSway)
	{
		return;
	}

	StopSway(); // para el del nivel anterior (con su blend out)
	CurrentSway = NewLevel;
	UE_LOG(LogTemp, Log, TEXT("[CombatCamera]: balanceo -> %s"), SwayName(CurrentSway));

	if (CurrentSway == ETimeSwayLevel::None)
	{
		return;
	}

	if (!GetSwayShake(CurrentSway))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCamera]: no hay shake asignado al nivel %s (Project Settings > Egnis > Camara)."),
			SwayName(CurrentSway));
		return;
	}

	PlaySway();

	// Si el shake dura menos que el nivel (p. ej. 0.25 s), se relanza cada cierto tiempo para que sea continuo
	const float Interval = GetDefault<UEgnisCameraSettings>()->SwayRestartInterval;
	if (Interval > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(SwayRestartHandle, this, &UCombatCameraSubsystem::PlaySway, Interval, true);
	}
}

void UCombatCameraSubsystem::PlaySway()
{
	const TSubclassOf<UCameraShakeBase> Shake = GetSwayShake(CurrentSway);
	if (!Shake)
	{
		return;
	}

	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
	{
		Cam->StartCameraShake(Shake);
	}
}

void UCombatCameraSubsystem::StopSway()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SwayRestartHandle);
	}

	if (CurrentSway == ETimeSwayLevel::None)
	{
		return;
	}

	const TSubclassOf<UCameraShakeBase> Shake = GetSwayShake(CurrentSway);
	if (Shake)
	{
		if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		{
			Cam->StopAllInstancesOfCameraShake(Shake, false); // false = con blend out, sin corte brusco
		}
	}
}

TSubclassOf<UCameraShakeBase> UCombatCameraSubsystem::GetSwayShake(ETimeSwayLevel Level) const
{
	const UEgnisCameraSettings* Settings = GetDefault<UEgnisCameraSettings>();
	switch (Level)
	{
	case ETimeSwayLevel::Low:  return Settings->TimeSwayLow.LoadSynchronous();
	case ETimeSwayLevel::Med:  return Settings->TimeSwayMed.LoadSynchronous();
	case ETimeSwayLevel::High: return Settings->TimeSwayHigh.LoadSynchronous();
	default:                   return nullptr;
	}
}

// ===== Daño a aliados =====

void UCombatCameraSubsystem::WatchAlly(ACharacterBase* Character)
{
	AAlly* Ally = Cast<AAlly>(Character);
	if (!IsValid(Ally))
	{
		return;
	}

	UAbilitySystemComponent* ASC = Ally->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	for (const FWatchedAttribute& W : Watched)
	{
		if (W.ASC.Get() == ASC)
		{
			return; // ya se escucha
		}
	}

	// El delegate de GAS salta tanto con los GE de las cartas como con ApplyPureDamage (SetNumericAttributeBase)
	const TWeakObjectPtr<AAlly> WeakAlly(Ally);
	auto Bind = [this, ASC, WeakAlly](const FGameplayAttribute& Attribute)
	{
		const FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(Attribute)
			.AddWeakLambda(this, [this, WeakAlly](const FOnAttributeChangeData& Data)
			{
				if (Data.NewValue < Data.OldValue)
				{
					PlayDamageShake(WeakAlly.Get(), Data.OldValue - Data.NewValue);
				}
			});

		FWatchedAttribute& Entry = Watched.AddDefaulted_GetRef();
		Entry.ASC = ASC;
		Entry.Attribute = Attribute;
		Entry.Handle = Handle;
	};

	Bind(UHealthAttributeSet::GetHealthAttribute());
	Bind(UHealthAttributeSet::GetShieldAttribute()); // un golpe absorbido por el escudo tambien sacude
}

void UCombatCameraSubsystem::PlayDamageShake(const AAlly* Ally, float Amount)
{
	if (!bCombatActive || !IsValid(Ally))
	{
		return;
	}

	const UEgnisCameraSettings* Settings = GetDefault<UEgnisCameraSettings>();
	const TSubclassOf<UCameraShakeBase> Shake = Settings->DamageShake.LoadSynchronous();
	if (!Shake)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCamera]: no hay Damage Shake asignado (Project Settings > Egnis > Camara)."));
		return;
	}

	// Escala segun el daño respecto a la vida maxima del aliado
	const float MaxHealth = FMath::Max(1.f, Ally->GetMaxHealth());
	const float Alpha = FMath::Clamp(Amount / MaxHealth / Settings->DamageFractionForMaxShake, 0.f, 1.f);
	const float Scale = FMath::Lerp(Settings->MinDamageShakeScale, Settings->MaxDamageShakeScale, Alpha);

	UE_LOG(LogTemp, Log, TEXT("[CombatCamera]: %s pierde %.1f, shake de dano a escala %.2f"), *Ally->GetName(), Amount, Scale);

	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
	{
		Cam->StartCameraShake(Shake, Scale);
	}
}

void UCombatCameraSubsystem::UnwatchAll()
{
	for (FWatchedAttribute& W : Watched)
	{
		if (UAbilitySystemComponent* ASC = W.ASC.Get())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(W.Attribute).Remove(W.Handle);
		}
	}
	Watched.Empty();
}