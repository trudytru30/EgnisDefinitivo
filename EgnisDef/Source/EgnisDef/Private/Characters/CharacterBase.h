#pragma once

#include "CoreMinimal.h"
#include "CoreAndSystems/Board.h"
#include "GameFramework/Character.h"
#include "CoreAndSystems/ColorType.h"
#include "CoreAndSystems/HealthAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "CharacterBase.generated.h"

UCLASS()
// Se añade IAbilitySystemInterface: es como GAS localiza el ASC de un actor sin necesidad
// de castear. BattleManager y las futuras GameplayAbility lo usarán para preguntar
// "¿qué ASC tiene este personaje?" sin saber si es AAlly, AEnemy, etc.
class EGNISDEF_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// El ASC es el "motor" de GAS: gestiona qué abilities tiene el personaje, qué GameplayEffects
	// tiene activos, y contiene una referencia al AttributeSet. Sin este campo, HealthAttributeSet
	// existe como objeto suelto pero nunca recibe GameplayEffects reales.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
	class UHealthAttributeSet* HealthAttributeSet;

	// Implementación obligatoria de la interfaz: así es como cualquier código externo
	// (BattleManager, AAlly::LossPoints, futuras abilities) obtiene el ASC de este actor
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	// ===== Funciones =====

	void HandleDeath();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Stats")
	int32 GetTeam();

	// ===== Grid / Board =====
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Grid")
	TObjectPtr<ABoard> Board = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	FTileCoord CurrentTile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	bool bSnapToTileOnBeginPlay = true;

	UPROPERTY(BlueprintReadOnly, Category="Grid")
	bool bHasMoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	float TileZOffset = 0.f;

	UFUNCTION(BlueprintCallable, Category="Grid")
	bool SetCurrentTile(const FTileCoord& NewTile);

	UFUNCTION(BlueprintCallable, Category="Grid")
	void SnapToCurrentTile(bool bKeepCurrentZ = false);

	
	// Sustituyen a HealthComp->GetCurrentHealth()/GetMaxHealth() de cara a Blueprint.
	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const;
	
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EColorType Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Team")
	int32 Team = 0;

private:
	bool bDeathHandled = false;
};