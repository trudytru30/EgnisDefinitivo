#pragma once

#include "CoreMinimal.h"
#include "BattleManager.generated.h"

class UAudioManager;
class AAlly;
struct FTileCoord;
class UDeckManager;
class UBaseCard;
class ACharacterBase;
class AEnemy;

// Enumerador de los posibles turnos
UENUM()
enum class ETurnEnum { PlayerTurn = 0, EnemyTurn = 1 };	// Para poder aniadir otros tipos en el futuro

UCLASS()
class EGNISDEF_API UBattleManager : public UObject
{
	GENERATED_BODY()

public:

	// Conexión con la UI para la mano
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerTurnStarted);

	//Evento que avisa del final del combate
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleEndedSignature, bool, bPlayerWon);

	UPROPERTY(BlueprintAssignable)
	FOnPlayerTurnStarted OnPlayerTurnStarted;

	UPROPERTY(BlueprintAssignable)
	FOnBattleEndedSignature OnBattleEndedEvent;

#pragma region Functions
	void SetDeckManager(UDeckManager* InDeckManager);
	void StartBattle();
	void StartPlayerTurn();
	void StartEnemyTurn();
	void EndTurn();
	bool PlayCard(UBaseCard* Card, AAlly* Character, ACharacterBase* TargetCharacter, FVector Location);
	void UpdateUnitsAlive();
	void EndBattle(bool bPlayerWon);
	UFUNCTION(BlueprintCallable, Category="Battle|Player")
	bool RequestMove(ACharacterBase* Unit, const FTileCoord& TargetTile);
	// Getters
	int32 GetTurnCount() const;
	TArray<ACharacterBase*> GetCharactersOnField() const;
	UFUNCTION(BlueprintPure)
	bool IsPlayerTurn() const;
#pragma endregion

private:

	// Avisan a los personajes de un bando de que empieza / termina su turno
	void NotifyTeamTurnStart(int32 Team);
	void NotifyTeamTurnEnd(int32 Team);
	
	// Salud actual de los aliados respecto a la suma de sus vidas maximas al empezar el combate (0 a 1).
	// Los aliados muertos cuentan como 0.
	float GetTeamHealthRatio() const;
	
#pragma region Private Variables
	// ===== Managers =====
	UPROPERTY()
	UDeckManager* DeckManager;

	// ===== Turn System =====
	int32 TurnCount = 0;
	ETurnEnum CurrentTurn = ETurnEnum::PlayerTurn;
	bool bBattleIsOver = false;

	// ===== Units =====
	UPROPERTY()
	TArray<ACharacterBase*> CharactersOnField;
	
	// Suma de la vida maxima de los aliados al empezar el combate (referencia de GetTeamHealthRatio)
	float InitialTeamMaxHealth = 0.0f;
#pragma endregion
};