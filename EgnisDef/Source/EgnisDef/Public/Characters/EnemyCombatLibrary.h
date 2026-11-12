#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoreAndSystems/Board.h"
#include "EnemyCombatLibrary.generated.h"

class AEnemy;
class ACharacterBase;

//acciones de los enemigos distancia en rombo
//obstaculos en medio
//tirada 33/66/100 del poder

//Que ha hecho el golpe de Ikwos
UENUM(BlueprintType)
enum class EEnemyStrikeResult : uint8
{
	None,
	Executed,
	HalvedHealth,
	Bled
};

UCLASS()
class EGNISDEF_API UEnemyCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//Tirada devuelve 0.33, 0.66 o 1.0 al azar
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	static float RollPercent();

	//Valor de una accion con la tirada aplicada
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	static float RollValue(float BaseValue);

	//Distancia en casillas sumando forma de rombo
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	static int32 TileDistance(const FTileCoord& A, const FTileCoord& B);

	//coprueba si hay algo entre from y to
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	static bool HasLineOfSight(const ABoard* Board, const FTileCoord& From, const FTileCoord& To);

	//rango de ataque en casillas (rombo) y si el objetivo es del equipo contrario
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	static bool CanAttackTarget(const AEnemy* Attacker, const ACharacterBase* Target);

	
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	static TArray<ACharacterBase*> GetHeroesInAttackRange(const AEnemy* Attacker);

	//Ataque: poder x tirada x bufo, con debilidad/expuesto. Devuelve el daño hecho (0 si no pudo)
	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static float EnemyAttack(AEnemy* Attacker, ACharacterBase* Target);

	//Escudo: poder x tirada, se acumula. Receptor vacio = a si mismo. Devuelve el escudo dado
	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static float EnemyShield(AEnemy* Caster, ACharacterBase* Receiver);

	//Bufo de daño en % durante X turnos propios. Receptor vacio = a si mismo
	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static bool EnemyDamageBuff(AEnemy* Caster, AEnemy* Receiver, float Percent = 10.f, int32 Turns = 1);


	//CURA
	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static float EnemyHeal(AEnemy* Caster, ACharacterBase* Receiver);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	static ACharacterBase* FindMostHurtAlly(AEnemy* Caster, float MaxHealthPercent = 1.f, bool bIncludeSelf = true);
	
	//ATAQUE AREA
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	static ACharacterBase* FindHeroByHealth(const AEnemy* Enemy, bool bMostHealth = true, bool bOnlyInAttackRange = true);
	
	// Atraccion Espiritual: El heroe vivo mas cercano con ese estado. Devuelve nullptr si el enemigo
	// no esta poseido por espiritus o nadie lo tiene. Con bOnlyInAttackRange solo cuenta si puede atacarle
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	static ACharacterBase* GetSpiritAttractionTarget(const AEnemy* Enemy, bool bOnlyInAttackRange = false);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	static int32 FindBestAreaCenter(const AEnemy* Attacker, int32 Radius, FTileCoord& OutCenter);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static float EnemyAreaAttack(AEnemy* Attacker, const FTileCoord& Center, int32 Radius = 1, bool bSplitDamage = false);

	//SANGRADO
	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static bool EnemyExecute(AEnemy* Attacker, ACharacterBase* Target, float ExecuteBelowPercent = 0.25f);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static float EnemyHalveHealth(AEnemy* Attacker, ACharacterBase* Target);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static bool EnemyApplyBleed(AEnemy* Attacker, ACharacterBase* Target, int32 Turns = 2);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static EEnemyStrikeResult EnemyExecutionStrike(AEnemy* Attacker, ACharacterBase* Target, float ExecuteBelowPercent = 0.25f, int32 BleedTurns = 2);

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	static float GetHealthPlusShieldPercent(const ACharacterBase* Character);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static float EnemyLoseShieldPercent(AEnemy* Enemy, float Percent = 50.f);

	//Taunt
	UFUNCTION(BlueprintCallable, Category = "Enemy|Actions")
	static int32 EnemyTaunt(AEnemy* Caster, int32 Turns = 1, int32 MaxDistance = 0);

	
	//MOV tanque
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static bool GetHeroesCenterTile(const AEnemy* Enemy, FTileCoord& OutCenter);
	
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static bool FindTileTowardsHeroesCenter(const AEnemy* Enemy, FTileCoord& OutTile);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static bool EnemyMoveToHeroesCenter(AEnemy* Enemy);
	
	
	//Casillas a las que puede llegar andando (4 direcciones, sin atravesar obstaculos ni personajes)
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static TArray<FTileCoord> GetReachableTiles(const AEnemy* Enemy);

	//Mueve al enemigo a esa casilla si puede llegar. False si no (paralizado, lejos, ocupada...)
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static bool EnemyMoveToTile(AEnemy* Enemy, const FTileCoord& Target);

	//Mejor casilla para ir a por el objetivo: si puede atacarle desde alguna, la de menos pasos;
	//si no, la que mas se le acerca. Puede devolver su propia casilla (ya esta a tiro o no puede mejorar)
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static bool FindTileTowards(const AEnemy* Enemy, const ACharacterBase* Target, FTileCoord& OutTile);

	//FindTileTowards + EnemyMoveToTile
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static bool EnemyMoveTowards(AEnemy* Enemy, ACharacterBase* Target);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	static bool EnemyMoveToLocation(AEnemy* Enemy, const FVector& Location);
	
private:
	static bool IsTileBlockingSight(const ABoard* Board, const FTileCoord& Tile);
	static void BuildReachableMap(const AEnemy* Enemy, TMap<FTileCoord, int32>& OutSteps);
};