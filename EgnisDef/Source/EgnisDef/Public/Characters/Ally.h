#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterBase.h"
#include "Components/GridMovementComponent.h"
#include "GameFramework/Character.h"
#include "CoreAndSystems/EnergyAttributeSet.h"
#include "Characters/HeroClassData.h"
#include "Ally.generated.h"

class UAudioManager;
class UBaseCard;

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

	UFUNCTION(BlueprintCallable, Category = "Setup")
	void InitializeFromData(UHeroDataAsset* HeroData);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
	class UEnergyAttributeSet* EnergyAttributeSet;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card Archetype")
	//EColorType ArchetypeColor = EColorType::Grey;
	TArray<EColorType> ArchetypeColors;

	UFUNCTION(BlueprintPure, Category="Card Archetype")
	//EColorType GetArchetypeColor() const { return ArchetypeColor; }
	bool HasArchetypeColor(EColorType Color) const { return ArchetypeColors.Contains(Color); }

	// Rango base del personaje. Se suma al rango de la carta.
	// Valor inicial editable; mas adelante se carga desde FHeroClassData::Range.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats|Range", meta=(ClampMin="0"))
	int32 BaseRange = 0;

	// Rango actual del personaje. Punto unico de lectura: las heridas, pasivas y reliquias
	// se aplican aqui dentro.
	UFUNCTION(BlueprintPure, Category="Stats|Range")
	int32 GetCharacterRange() const;

	// Rango con el que puede jugar una carta: rango de la carta + rango del personaje.
	UFUNCTION(BlueprintPure, Category="Stats|Range")
	int32 GetEffectiveRange(const UBaseCard* Card) const;
	
	UFUNCTION(BlueprintPure, Category="Energy")
	int32 GetCurrentEnergy() const;

	UFUNCTION(BlueprintPure, Category="Energy")
	int32 GetMaxEnergy() const;
	
protected:
	virtual void BeginPlay() override;
};