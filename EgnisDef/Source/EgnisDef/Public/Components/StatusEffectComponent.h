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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float BurnDamage = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float BleedDamage = 2.f;
	
	// Rastro de la Quemadura: turnos que dura cada casilla en llamas (sin contar el turno en que se crea)
	// y daño puro que hace al pisarla. Editables por diseño.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "1"))
	int32 BurnTrailTurns = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0"))
	float BurnTrailDamage = 2.f;
	
	// Fatigado: el coste de las cartas de este personaje se multiplica por esto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "1"))
	int32 FatigueCostMultiplier = 2;
	
	// Espinas: fraccion del daño recibido que se refleja al atacante
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0", ClampMax = "1"))
	float ThornsReflectFraction = 0.5f;

	// Espinas: true redondea el daño reflejado hacia arriba, false hacia abajo
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	bool bThornsRoundUp = true;
	
	// Maldicion Ignea: cargas que hacen falta para matar al objetivo
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "1"))
	int32 CurseChargesToKill = 4;

	// Maldicion Ignea: probabilidad de que cada carga se aplique = CurseMinChance + % de vida
	// que le falta al objetivo (hasta CurseMaxChance). Con la vida llena es CurseMinChance.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0", ClampMax = "1"))
	float CurseMinChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0", ClampMax = "1"))
	float CurseMaxChance = 1.0f;
	
	// Enraizado: porcentaje de la vida maxima que cura en el primer turno que cura (0.2 = 20%)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0", ClampMax = "1"))
	float RootedInitialHealPercent = 0.2f;

	// Enraizado: cuanto baja ese porcentaje en cada turno que cura (0.05 = 5 puntos menos por turno)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0", ClampMax = "1"))
	float RootedHealReductionPerTurn = 0.05f;
	
	// Ralentizado: Fraccion del movimiento que se conserva. Se redondea hacia arriba y nunca baja de 1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0", ClampMax = "1"))
	float SlowMovementFactor = 0.5f;
	
	// Escurridizo: false = solo atraviesa obstaculos; true = tambien unidades (aliadas y enemigas)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	bool bElusiveAlsoPassesUnits = true;
	
	// Perfumar: true = los enemigos no poseidos SOLO pueden atacar al perfumado.
	// false = lo prefieren si esta a su alcance, pero pueden atacar a otros
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	bool bPerfumeExclusive = false;
	
	// Locura: cuantas cartas juega al azar al empezar su turno (si tiene menos jugables, juega las que haya)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0"))
	int32 MadnessCardsPerTurn = 3;
	
	// Locura: segundos que espera tras empezar el turno antes de actuar (que de tiempo a repartir la mano)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.0"))
	float MadnessStartDelay = 1.2f;

	// Locura: segundos entre una accion (moverse o jugar una carta) y la siguiente
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.0"))
	float MadnessActionDelay = 0.8f;

	// Locura: segundos que se ve la carta elegida en la mano antes de jugarse
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.0"))
	float MadnessCardPreviewDelay = 0.5f;
	
	// Sangre hirviente: daño puro por accion o movimiento. Empieza en Initial y baja Reduction por cada turno
	// que ha pasado desde que se aplico, sin bajar de Min.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0"))
	float BoilingBloodInitialDamage = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0"))
	float BoilingBloodReductionPerTurn = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0"))
	float BoilingBloodMinDamage = 1.f;
	
	// Cenizas: añade cargas. No usa turnos
	UFUNCTION(BlueprintCallable, Category = "Status")
	bool ApplyAshes(int32 Stacks, ACharacterBase* Source = nullptr);

	// Sangre hirviente: daño que haria ahora una accion o movimiento
	UFUNCTION(BlueprintPure, Category = "Status")
	float GetBoilingBloodDamage() const;

	// Lo llama BattleManager cuando el afectado juega una carta o se mueve
	void OnOwnerActionTaken(const TCHAR* ActionName);
	
	// Si el estado ya estaba activo, se queda con la duracion mayor (no se suman turnos de momento)
	UFUNCTION(BlueprintCallable, Category = "Status")
	bool ApplyStatus(EStatusType Type, int32 Turns, ACharacterBase* Source = nullptr,
		EStatusApplicationTurn ApplicationTurn = EStatusApplicationTurn::Default);
	
	// Maldicion Ignea: intenta aplicar 'Charges' cargas, cada una con su propia tirada de probabilidad.
	// Devuelve cuantas se aplicaron. Al llegar a CurseChargesToKill el personaje muere.
	UFUNCTION(BlueprintCallable, Category = "Status")
	int32 ApplyFieryCurse(int32 Charges, ACharacterBase* Source = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void RemoveStatus(EStatusType Type);

	UFUNCTION(BlueprintPure, Category = "Status")
	bool HasStatus(EStatusType Type) const;

	UFUNCTION(BlueprintPure, Category = "Status")
	int32 GetTurnsLeft(EStatusType Type) const;
	
	// Cargas acumuladas de un estado (0 si no esta activo)
	UFUNCTION(BlueprintPure, Category = "Status")
	int32 GetStacks(EStatusType Type) const;
	
	// Valor numerico fijado al aplicar el estado. 0 si no esta activo
	UFUNCTION(BlueprintPure, Category = "Status")
	float GetStatusMagnitude(EStatusType Type) const;

	UFUNCTION(BlueprintPure, Category = "Status")
	ACharacterBase* GetStatusSource(EStatusType Type) const;
	
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
	// Descuenta un turno a los estados
	void ConsumeTurns();
	
	void ApplyBurnDamage();
	
	// Cenizas: bloquea el estado entrante y gasta una carga.
	bool TryBlockWithAshes(EStatusType Incoming);
};