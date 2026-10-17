#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StatusEffectTypes.h"
#include "StatusEffectComponent.generated.h"

class ACharacterBase;
class UAbilitySystemComponent;

// Gestiona los estados alterados de un personaje. GAS mide las duraciones en segundos y
// aqui se cuentan en turnos, asi que la cuenta vive en este componente. Lo que si pasa
// por GAS son los GameplayTags: mientras un estado esta activo, su tag esta en el ASC del
// personaje, y cualquier sistema puede consultarlo con HasMatchingGameplayTag.
UCLASS(ClassGroup = (Egnis), meta = (BlueprintSpawnableComponent))
class EGNISDEF_API UStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusEffectComponent();

	// Aviso para la UI (iconos de estado)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatusChanged);

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FOnStatusChanged OnStatusChanged;

	// Si el estado ya estaba activo, se queda con la duracion mayor (no se suman turnos de momento)
	UFUNCTION(BlueprintCallable, Category = "Status")
	bool ApplyStatus(EStatusType Type, int32 Turns, ACharacterBase* Source = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void RemoveStatus(EStatusType Type);

	UFUNCTION(BlueprintPure, Category = "Status")
	bool HasStatus(EStatusType Type) const;

	UFUNCTION(BlueprintPure, Category = "Status")
	int32 GetTurnsLeft(EStatusType Type) const;

	UFUNCTION(BlueprintPure, Category = "Status")
	TArray<FActiveStatus> GetActiveStatuses() const { return ActiveStatuses; }

	// Inicio y final del turno del bando del personaje (los llama BattleManager)
	void OnOwnerTurnStart();
	void OnOwnerTurnEnd();

private:
	UPROPERTY()
	TArray<FActiveStatus> ActiveStatuses;

	// True mientras es el turno del bando del personaje
	bool bOwnerTurnActive = false;

	FActiveStatus* FindStatus(EStatusType Type);
	const FActiveStatus* FindStatus(EStatusType Type) const;
	UAbilitySystemComponent* GetOwnerASC() const;
};