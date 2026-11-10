#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemy.h"
#include "DaewaBoss.generated.h"

// Lado del tablero (por fuera) en el que esta Daewa.
// Si en pantalla gira al reves de las agujas del reloj, marcar bReverseDirection en el BP
UENUM(BlueprintType)
enum class EDaewaSide : uint8
{
	XMax UMETA(DisplayName = "Lado X maximo"),
	YMax UMETA(DisplayName = "Lado Y maximo"),
	XMin UMETA(DisplayName = "Lado X minimo"),
	YMin UMETA(DisplayName = "Lado Y minimo")
};

//En que punto del lado se coloca (de eso depende que 2 filas/columnas ataca)
UENUM(BlueprintType)
enum class EDaewaLinePlacement : uint8
{
	InFrontOfHeroes UMETA(DisplayName = "Delante de los heroes"), // donde el ataque pille a mas heroes
	Center          UMETA(DisplayName = "Centro del lado"),
	Random          UMETA(DisplayName = "Al azar")
};

//Donde aparecen los enemigos que invoca
UENUM(BlueprintType)
enum class EDaewaSummonPlacement : uint8
{
	Random        UMETA(DisplayName = "Al azar"),
	NearHeroes    UMETA(DisplayName = "Cerca de los heroes"),
	FarFromHeroes UMETA(DisplayName = "Lejos de los heroes")
};

//Un enemigo que Daewa puede invocar
USTRUCT(BlueprintType)
struct FDaewaSummonEntry
{
	GENERATED_BODY()

	//AJUSTABLE: BP del enemigo (BP_Enemy_Apero, BP_Enemy_Ewito...)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa")
	TSubclassOf<AEnemy> EnemyClass;

	//AJUSTABLE: vida que pierde Daewa al invocar a este enemigo
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa", meta = (ClampMin = "0"))
	float HealthCost = 10.f;

	//AJUSTABLE: probabilidad relativa de salir (1 = normal, 2 = el doble, 0 = nunca)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa", meta = (ClampMin = "0"))
	int32 Weight = 1;
};


// Todo lo marcado con AJUSTABLE se cambia en el BP del boss (Class Defaults>categoria "Daewa")
//Funciona sin Behavior Tree (todo su turno va en MakeAction)
//Si se le pone BT, llamar a DaewaDoTurn y luego Fin de turno
UCLASS()
class EGNISDEF_API ADaewaBoss : public AEnemy
{
	GENERATED_BODY()

public:
	ADaewaBoss();

	//---------- VIDA ----------

	//AJUSTABLE:vida con la que aparece (aparece en pantalla),su vida max
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Vida", meta = (ClampMin = "1"))
	float InitialHealth = 999.f;

	//AJUSTABLE:vida con la que se queda en cuanto empieza el combate (despues de desplegar)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Vida", meta = (ClampMin = "1"))
	float HealthAfterDeploy = 60.f;

	//---------- INVOCAR ----------

	//AJUSTABLE:enemigos que puede invocar y cuanta vida le cuesta cada uno
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Invocar")
	TArray<FDaewaSummonEntry> SummonList;

	//AJUSTABLE:cuantos enemigos tiene que haber siempre en el tablero (sin contarle a el)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Invocar", meta = (ClampMin = "0"))
	int32 MaxEnemiesOnBoard = 4;

	//AJUSTABLE:donde aparecen los enemigos invocados
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Invocar")
	EDaewaSummonPlacement SummonPlacement = EDaewaSummonPlacement::Random;

	//AJUSTABLE: distancia minima (en casillas) a cualquier heroe para aparecer
	//0 = sin limite
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Invocar", meta = (ClampMin = "0"))
	int32 MinDistanceToHeroes = 2;

	//---------- MOVIMIENTO ----------

	//AJUSTABLE:lado en el que empieza
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Movimiento")
	EDaewaSide StartSide = EDaewaSide::XMax;

	//AJUSTABLE:marcarlo si en pantalla gira al reves de las agujas del reloj
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Movimiento")
	bool bReverseDirection = false;

	//AJUSTABLE:en que punto del lado se coloca (decide que 2 filas/columnas ataca)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Movimiento")
	EDaewaLinePlacement LinePlacement = EDaewaLinePlacement::InFrontOfHeroes;

	//AJUSTABLE:a cuantas casillas por fuera del borde se pone (solo visual)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Movimiento", meta = (ClampMin = "1"))
	int32 DistanceOutsideBoard = 1;

	//---------- ATAQUE EN LINEA ----------

	// AJUSTABLE: cada cuantos turnos suyos cae el ataque (1 = todos los turnos)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Ataque", meta = (ClampMin = "1"))
	int32 AttackEveryNTurns = 2;

	//AJUSTABLE:si avisa un turno antes marcando las casillas (el jugador puede apartarse)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Ataque")
	bool bWarnBeforeLineAttack = true;

	//AJUSTABLE:poder del ataque (el daño es poder x tirada 33/66/100, como el resto)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Ataque", meta = (ClampMin = "0"))
	float LineAttackPower = 9.f;

	//AJUSTABLE:si el ataque tambien daña a los enemigos que esten en medio
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Ataque")
	bool bDamagesOwnEnemies = false;

	//AJUSTABLE (pruebas):pinta en rojo las casillas avisadas (cajas de DEBUG) hasta que cae el ataque
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Daewa|Ataque")
	bool bDrawDebugLineAttack = true;

	//---------- ACCIONES (para el BT o para probar) ----------

	//Turno completo:cae el ataque avisado, rellena enemigos, cambia de lado y avisa del siguiente ataque
	UFUNCTION(BlueprintCallable, Category = "Daewa")
	void DaewaDoTurn();

	//Pasa al siguiente lado (sentido de las agujas del reloj) y se coloca segun LinePlacement
	UFUNCTION(BlueprintCallable, Category = "Daewa")
	bool DaewaMoveToNextSide();

	//Invoca hasta que haya MaxEnemiesOnBoard enemigos
	//Pierde la vida de cada uno
	//Devuelve cuantos ha invocado
	UFUNCTION(BlueprintCallable, Category = "Daewa")
	int32 DaewaSummonUpToMax();

	//Marca las 2 filas/columnas que va a atacar (aviso)
	UFUNCTION(BlueprintCallable, Category = "Daewa")
	bool DaewaMarkLineAttack();

	//Lanza el ataque sobre las casillas marcadas (o las de delante si no habia aviso)
	//Devuelve el daño total
	UFUNCTION(BlueprintCallable, Category = "Daewa")
	float DaewaFireLineAttack();

	//Baja la vida a HealthAfterDeploy (se hace sola al empezar el combate)
	UFUNCTION(BlueprintCallable, Category = "Daewa")
	void DaewaApplyDeployHealthDrop();

	//Si se ha quedado sin vida, desaparece ya (lo usan los comandos de prueba)
	UFUNCTION(BlueprintCallable, Category = "Daewa")
	void DaewaFinishIfDead();

	UFUNCTION(BlueprintPure, Category = "Daewa")
	int32 CountEnemiesOnBoard() const;

	UFUNCTION(BlueprintPure, Category = "Daewa")
	EDaewaSide GetCurrentSide() const { return CurrentSide; }

	UFUNCTION(BlueprintPure, Category = "Daewa")
	TArray<FTileCoord> GetPendingAttackTiles() const { return PendingAttackTiles; }

	UFUNCTION(BlueprintPure, Category = "Daewa")
	bool IsOutOfHealth() const { return bPendingDeath; }

	//----------CONEXION PARA ANIMACIONES Y EFECTOS----------

	UFUNCTION(BlueprintImplementableEvent, Category = "Daewa|Animation")
	void OnDaewaSideChanged(EDaewaSide NewSide, int32 Position);

	UFUNCTION(BlueprintImplementableEvent, Category = "Daewa|Animation")
	void OnDaewaLineAttackMarked(const TArray<FTileCoord>& Tiles);

	UFUNCTION(BlueprintImplementableEvent, Category = "Daewa|Animation")
	void OnDaewaLineAttackFired(const TArray<FTileCoord>& Tiles, float TotalDamage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Daewa|Animation")
	void OnDaewaSummoned(AEnemy* Summoned, float HealthCost);

	UFUNCTION(BlueprintImplementableEvent, Category = "Daewa|Animation")
	void OnDaewaDeployHealthDrop(float OldHealth, float NewHealth);

	//---------- Comportamiento sin BT ----------
	virtual void MoveTowardClosesPlayer() override;
	virtual void MakeAction() override;
	virtual void OnOwnTurnFinished() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	EDaewaSide CurrentSide = EDaewaSide::XMax;
	int32 CurrentPosition = 0;
	int32 TurnsSinceLastAttack = 0;
	float BaseZ = 0.f;
	bool bDeployDropDone = false;
	bool bPendingDeath = false;
	TArray<FTileCoord> PendingAttackTiles;

	void Die();
	void SetHealthValue(float NewHealth);
	void PayHealth(float Cost);

	int32 GetSideLength(EDaewaSide Side) const;
	int32 ChooseLinePosition(EDaewaSide Side) const;
	void PlaceOnSide(EDaewaSide Side, int32 Position);
	void GetLineTiles(EDaewaSide Side, int32 Position, TArray<FTileCoord>& OutTiles) const;
	void ClearAttackDebug();

	const FDaewaSummonEntry* PickSummonEntry() const;
	bool PickSummonTile(FTileCoord& OutTile) const;
	AEnemy* SpawnSummon(TSubclassOf<AEnemy> EnemyClass, const FTileCoord& Tile);
};
