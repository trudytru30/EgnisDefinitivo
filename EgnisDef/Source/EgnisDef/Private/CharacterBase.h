#pragma once

#include "CoreMinimal.h"
// TODO: Descomentar cuando se migre ABoard / FTileCoord al nuevo proyecto
// #include "Board.h"
#include "GameFramework/Character.h"
// TODO: Descomentar cuando se migre EColorType al nuevo proyecto
// #include "ColorType.h"
#include "HealthComponent.h"
#include "GridMovementComponent.h"
#include "CharacterBase.generated.h"

// De momento no se usa: UEnergyComponent aun no existe en el proyecto nuevo
// class UEnergyComponent;

UCLASS()
class EGNISDEF_API ACharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	UHealthComponent* HealthComp;

	// Segun el GDD todas las tropas (aliadas y enemigas) se mueven con rango+patron,
	// no hay personajes estaticos, asi que se engancha igual que HealthComp.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	UGridMovementComponent* MovementComp;

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
	// Todo este bloque depende de ABoard / FTileCoord, que de momento no existen en este proyecto.
	// Descomentar al migrar Board.

	// UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Grid")
	// TObjectPtr<ABoard> Board = nullptr;

	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	// FTileCoord CurrentTile;

	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	// bool bSnapToTileOnBeginPlay = true;

	// UPROPERTY(BlueprintReadOnly, Category="Grid")
	// bool bHasMoved = false;

	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	// float TileZOffset = 0.f;

	// UFUNCTION(BlueprintCallable, Category="Grid")
	// bool SetCurrentTile(const FTileCoord& NewTile);

	// UFUNCTION(BlueprintCallable, Category="Grid")
	// void SnapToCurrentTile(bool bKeepCurrentZ = false);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// De momento no se usa: depende de ColorType.h
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// EColorType Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Team")
	int32 Team = 0; // 0 para el jugador, 1 para los enemigos, etc.
};