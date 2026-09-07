#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterBase.h"
#include "Components/GridMovementComponent.h"
#include "GameFramework/Character.h"
#include "Ally.generated.h"

class UAudioManager;
class UEnergyComponent;


UCLASS()
class EGNISDEF_API AAlly : public ACharacterBase
{
	GENERATED_BODY()

public:
	AAlly();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void LossPoints(int32 Cost);
	void GainPoints(int32 Bonus);
	void ResetEnergyForTurn();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	UEnergyComponent* EnergyComp;

	// Componente de movimiento en grid: va aqui, en Ally, y no en CharacterBase — AEnemy no lo usa
	// (tiene su propia logica de movimiento, ver Enemy::MoveTowardClosesPlayer), y BattleManager::
	// RequestMove ya lo busca dinamicamente con FindComponentByClass, asi que solo hace falta que
	// exista en las unidades que realmente lo necesitan.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UGridMovementComponent* MoveComp;

	// Aarquetipo/color de este aliado
	// Reutiliza EColorType (ya
	// definido para las cartas: Grey=comun, Turquoise=area, Indigo=utilidad, DarkMagenta=ataque
	// fuerte, Vermillion=ataque ligero)
	// Lo usa DeckManager::IsCardArchetypeAvailable para
	// saber si hay que pintar en gris una carta cuyo arquetipo no tiene dueño en el equipo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card Archetype")
	EColorType ArchetypeColor = EColorType::Grey;

	UFUNCTION(BlueprintPure, Category="Card Archetype")
	EColorType GetArchetypeColor() const { return ArchetypeColor; }

protected:
	virtual void BeginPlay() override;
};