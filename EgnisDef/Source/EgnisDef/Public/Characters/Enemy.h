#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterBase.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Enemy.generated.h"

class UAudioManager;
enum class EActionId : uint8;
class UActionDataAsset;
class UEnemyArchetypeDataAsset;


//Que accion ha hecho el enemigo(para animaciones / efectos)
UENUM(BlueprintType)
enum class EEnemyActionType : uint8
{
	Attack,
	AreaAttack,
	Shield,
	Heal,
	DamageBuff,
	Execute,
	HalveHealth,
	Bleed,
	Taunt,
	LoseShield,
	Summon,//Daewa invoca (target = el invocado, Amount = vida que le cuesta)
	LineAttackWarning,//Daewa avisa del ataque (Amount = nº de casilla)
	LineAttack//Daewa lanza el ataque (Amount = daño total)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnEnemyActionDone, AEnemy*, Enemy, EEnemyActionType, Action, ACharacterBase*, Target, float, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnEnemyMovedDone, AEnemy*, Enemy, FTileCoord, From, FTileCoord, To, int32, Steps);

UCLASS()
class EGNISDEF_API AEnemy : public ACharacterBase
{
	GENERATED_BODY()

public:

	AEnemy();

	//estadisticas de la tabla
	//mira que fila de la tabla es este enemigo
	//Si se deja vacía, usa los valores de siempre
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Stats", meta = (RowType = "/Script/EgnisDef.EnemyStatsRow"))
	FDataTableRowHandle StatsRow;

	// Valores ya aplicados (se rellenan al aparecer)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Stats")
	float Power = 10.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Stats")
	int32 MoveRange = 2;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Stats")
	int32 AttackRange = 1;

	UFUNCTION(BlueprintPure, Category = "Enemy|Stats")
	float GetPower() const { return Power; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Stats")
	int32 GetMoveRange() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Stats")
	int32 GetAttackRange() const { return AttackRange; }

	//Behaviour tree
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI")
	TObjectPtr<class UBehaviorTree> BehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	bool bAttacksOverObstacles = false;	
	
	virtual void Tick(float DeltaTime) override;
	
	//Selecionar Arquetipo
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Enemy|Data")
	TObjectPtr<UEnemyArchetypeDataAsset> ArchetypeData = nullptr;

	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category ="Enemy|AI")
	bool bDebugMovementOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Combat")
	float HealAmount = 8.f;

	// Mismo GE_CardDamage reutilizado ya en las cartas — Add + Set By Caller sirve igual
	// para el ataque (negativo) que para la curación (positivo) del enemigo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat")
	TSubclassOf<class UGameplayEffect> AttackEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat")
	TSubclassOf<class UGameplayEffect> HealEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat")
	TSubclassOf<class UGameplayEffect> ShieldEffect;

	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Enemy|Combat")
	float DamageBuffPercent = 0.f;

	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Enemy|Combat")
	int32 DamageBuffTurnsLeft = 0;

	
	UFUNCTION(BlueprintPure, Category="Enemy|Combat")
	float GetDamageMultiplier() const { return 1.f + (DamageBuffTurnsLeft > 0 ? DamageBuffPercent : 0.f) / 100.f; }

	
	void AddDamageBuff(float Percent, int32 Turns, bool bAppliedDuringOwnTurn);

	//Lo llama el AIController al terminar el turno de este enemigo
	virtual void OnOwnTurnFinished();	


	//Orden en el turno enemigo: los numeros mas bajos actuan antes (empate = orden de aparicion)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	int32 TurnOrder = 0;

	//conexiones para animaciones / efectos
	//Se implementan en el BP del enemigo (Add Event)

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Animation")
	void OnEnemyTurnStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Animation")
	void OnEnemyTurnEnded();

	//Ya esta en la casilla nueva (el movimiento es instantaneo): From/To/Steps para animar el recorrido
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Animation")
	void OnEnemyMoved(FTileCoord From, FTileCoord To, int32 Steps);

	//Accion hecha
	//Target puede ser nulo (area, taunt) o el propio enemigo (escudo/cura a si mismo)
	//Amount = daño, escudo, cura, nº de heroes
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Animation")
	void OnEnemyAction(EEnemyActionType Action, ACharacterBase* Target, float Amount);

	//Lo mismo, para engancharse desde fuera (UI, camara, etc)
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Animation")
	FOnEnemyActionDone OnActionDone;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Animation")
	FOnEnemyMovedDone OnMovedDone;

	//Lo llama la libreria de acciones
	void NotifyAction(EEnemyActionType Action, ACharacterBase* Target, float Amount);
	void NotifyMoved(const FTileCoord& From, const FTileCoord& To, int32 Steps);	

	
	UFUNCTION(BlueprintCallable, Category ="Enemy|AI")
	virtual void MoveTowardClosesPlayer();

	UFUNCTION(BlueprintCallable, Category ="Enemy|AI")
	virtual void MakeAction();

	UFUNCTION(BlueprintCallable,Category = "Enemy|Actions")
	const UActionDataAsset* GetActionById(EActionId ActionId) const;

protected:

	virtual void BeginPlay() override;

private:

	ACharacterBase* FindClosestPlayer() const;
	static int32 ManhattanDistance(const FTileCoord& A, const FTileCoord& B);

	ACharacterBase* FindBestTarget(int32 _Range, int32 _BestRatio, int32 _Team);

	bool ApplyStatsFromTable();
	bool bSkipNextBuffDecrement = false;
};
