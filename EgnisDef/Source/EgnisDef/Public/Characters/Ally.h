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
class UStaticMeshComponent;
class UHeroDataAsset;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TObjectPtr<UHeroDataAsset> HeroAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
	class UEnergyAttributeSet* EnergyAttributeSet;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
	class UCharacterStatsAttributeSet* StatsAttributeSet;

	// Componente de movimiento en grid: va aqui, en Ally, y no en CharacterBase — AEnemy no lo usa
	// (tiene su propia logica de movimiento, ver Enemy::MoveTowardClosesPlayer), y BattleManager::
	// RequestMove ya lo busca dinamicamente con FindComponentByClass, asi que solo hace falta que
	// exista en las unidades que realmente lo necesitan.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UGridMovementComponent* MoveComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* ModelMesh;

	// Aarquetipo/color de este aliado
	// Reutiliza EColorType (ya
	// definido para las cartas: Grey=comun, Turquoise=area, Indigo=utilidad, DarkMagenta=ataque
	// fuerte, Vermillion=ataque ligero)
	// Lo usa DeckManager::IsCardArchetypeAvailable para
	// saber si hay que pintar en gris una carta cuyo arquetipo no tiene dueño en el equipo.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cults")
	//EColorType ArchetypeColor = EColorType::Grey;
	FGameplayTagContainer Cults;

	UFUNCTION(BlueprintPure, Category="Cults")
	bool HasCult(FGameplayTag Cult) const { return Cults.HasTag(Cult); }

	UFUNCTION(BlueprintPure, Category="Card Archetype", meta=(DeprecatedFunction, DeprecationMessage="Usa CanBePlayedBy de la carta o HasCult"))
	//EColorType GetArchetypeColor() const { return ArchetypeColor; }
	bool HasArchetypeColor(EColorType Color) const;

	// Rango actual del personaje. Punto unico de lectura: las heridas, pasivas y reliquias
	// se aplican aqui dentro.
	UFUNCTION(BlueprintPure, Category="Stats|Range")
	int32 GetCharacterRange() const;

	// Rango con el que puede jugar una carta: rango de la carta + rango del personaje.
	UFUNCTION(BlueprintPure, Category="Stats|Range")
	int32 GetEffectiveRange(const UBaseCard* Card) const;

	// Piedad actual. Punto unico de lectura: heridas (Perforacion), pasivas y reliquias
	// se aplicaran aqui dentro, igual que con el rango.
	UFUNCTION(BlueprintPure, Category="Stats|Piety")
	float GetCharacterPiety() const;
	
	UFUNCTION(BlueprintPure, Category="Energy")
	int32 GetCurrentEnergy() const;

	UFUNCTION(BlueprintPure, Category="Energy")
	int32 GetMaxEnergy() const;
	
	// Power y velocidad actuales
	UFUNCTION(BlueprintPure, Category="Stats|Power")
	float GetCharacterPower() const;

	UFUNCTION(BlueprintPure, Category="Stats|Speed")
	int32 GetCharacterSpeed() const;
	
protected:
	virtual void BeginPlay() override;
	
	// Mantiene GridMovementComponent::moveRange igual que el atributo Speed
	void RefreshMoveRange();
	void OnSpeedChanged(const struct FOnAttributeChangeData& Data);
};