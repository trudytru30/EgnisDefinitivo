#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RunModifierSubsystem.generated.h"

class AAlly;
class ACharacterBase;

UENUM(BlueprintType)
enum class ERunModifierStat : uint8
{
	Power,
	Piety,
	Speed,
	Range
};

/** Buffo o debuffo de la run que dura varios combates. */
USTRUCT(BlueprintType)
struct FRunModifier
{
	GENERATED_BODY()

	// A quién afecta: un héroe concreto (HeroId), un culto, o todos si ambos están vacíos
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Modifier")
	FName HeroId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Modifier")
	FGameplayTag Cult;

	// Cambio por stat (negativo = debuffo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Modifier")
	TMap<ERunModifierStat, float> StatDeltas;

	// Multiplicador del daño que HACE el héroe afectado (0.5 = hace la mitad, 2 = el doble)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Modifier")
	float DamageDealtMultiplier = 1.f;

	// Multiplicador del daño que RECIBE el héroe afectado (1.5 = recibe un 50% más)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Modifier")
	float DamageTakenMultiplier = 1.f;

	// Combates que le quedan (se gastan se gane o se pierda)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run Modifier")
	int32 CombatsLeft = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunModifiersChanged);

UCLASS()
class EGNISDEF_API URunModifierSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Run Modifier")
	FOnRunModifiersChanged OnModifiersChanged;

	UFUNCTION(BlueprintCallable, Category = "Run Modifier")
	void AddModifier(const FRunModifier& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Run Modifier")
	void ClearAll();

	UFUNCTION(BlueprintPure, Category = "Run Modifier")
	const TArray<FRunModifier>& GetModifiers() const { return Modifiers; }

	/** Suma los modificadores que le tocan a este aliado (al empezar el combate, tras desplegar). */
	void ApplyToAlly(AAlly* Ally, FName HeroId);

	/** Gasta un combate a cada modificador y quita los que se acaban (al terminar el combate). */
	void ConsumeCombat();

	/** Multiplicadores del combate en curso (1 si el personaje no tiene ninguno). Los usa CalculateDamage. */
	float GetDamageDealtMultiplier(const ACharacterBase* Character) const;
	float GetDamageTakenMultiplier(const ACharacterBase* Character) const;

	void DumpToLog() const;

private:
	UFUNCTION()
	void HandleRunChanged();

	UPROPERTY()
	TArray<FRunModifier> Modifiers;

	// Multiplicadores ya resueltos por personaje, solo durante el combate actual
	TMap<TWeakObjectPtr<const ACharacterBase>, FVector2D> CombatMultipliers; // X = hecho, Y = recibido
};