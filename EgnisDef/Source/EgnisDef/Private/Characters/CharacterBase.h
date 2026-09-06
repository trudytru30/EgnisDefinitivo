#pragma once

#include "CoreMinimal.h"
#include "CoreAndSystems/Board.h" // Board ya migrado (task #3): FTileCoord y ABoard disponibles.
#include "GameFramework/Character.h"
#include "CoreAndSystems/ColorType.h"
#include "Components/HealthComponent.h"
#include "CharacterBase.generated.h"



UCLASS()
class EGNISDEF_API ACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	UHealthComponent* HealthComp;

	// NOTA (corregido tras revisar la rama progra): el movimiento NO va aqui.
	// En el codigo real, UGridMovementComponent solo se engancha en AAlly (ver AAlly::MoveComp).
	// AEnemy no lo usa: se mueve con logica propia (Manhattan distance) en Enemy::MoveTowardClosesPlayer.
	// BattleManager::RequestMove lo busca con Unit->FindComponentByClass<UGridMovementComponent>(),
	// asi que solo hace falta que exista en las unidades que realmente lo necesitan.

	// ===== Funciones =====
	UFUNCTION(BlueprintCallable, Category="Stats")
	void LossHealth(float HealthToLoss);

	void HandleDeath();

	UFUNCTION(BlueprintCallable, Category="Stats")
	void GainHealth(float AmountHealed);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Stats")
	int32 GetTeam();

	// ===== Grid / Board =====
	// Reconciliado con el Board real (task #4). BattleManager::RequestMove usa
	// Unit->Board, Unit->bHasMoved, Unit->SetCurrentTile y Unit->SnapToCurrentTile
	// directamente (no pasa por GridMovementComponent::moveToTile), asi que esto
	// tenia que descomentarse a la vez que GridMovementComponent.

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Grid")
	TObjectPtr<ABoard> Board = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	FTileCoord CurrentTile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	bool bSnapToTileOnBeginPlay = true;

	// La lee y escribe BattleManager (RequestMove / StartPlayerTurn) para permitir un solo movimiento por turno.
	UPROPERTY(BlueprintReadOnly, Category="Grid")
	bool bHasMoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	float TileZOffset = 0.f;

	UFUNCTION(BlueprintCallable, Category="Grid")
	bool SetCurrentTile(const FTileCoord& NewTile);

	UFUNCTION(BlueprintCallable, Category="Grid")
	void SnapToCurrentTile(bool bKeepCurrentZ = false);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EColorType Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Team")
	int32 Team = 0; // 0 para el jugador, 1 para los enemigos, etc.

private:
	// PARCHE TEMPORAL (task #16, ver nota en HealthComponent::OnDeath): guarda de seguridad para
	// que HandleDeath() no se ejecute dos veces si algo lo llamara por partida doble. Se sustituira
	// cuando la muerte pase a gestionarse desde GAS.
	bool bDeathHandled = false;
};