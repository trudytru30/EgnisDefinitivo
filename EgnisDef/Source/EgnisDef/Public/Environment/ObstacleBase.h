#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoreAndSystems/Board.h"
#include "AbilitySystemInterface.h"
#include "ObstacleBase.generated.h"

// De momento esta clase cubre solo los dos obstaculos que necesitamos ya: uno que
// simplemente bloquea el paso (BP_Obstacle_Rock, bDestructible=false) y uno destructible
//Los demas obstaculos no estan cubiertos y habria que ampliar codigo
//En ese caso se haria un bP_ObstacleBase padre como ally
//Rock y tronco van con false y true en destructible(prueba:tronco tiene vida a 20)
class UHealthComponent;
class UAbilitySystemComponent;
class UHealthAttributeSet;

UCLASS()
class EGNISDEF_API AObstacleBase : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AObstacleBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	UHealthComponent* HealthComp;

	// TODO: HealthComp queda solo por compatibilidad con los Blueprints antiguos (el tronco tiene 20 puesto ahi).
	// La vida real ahora es HealthAttributeSet; cuando los BPs usen InitialHealth, HealthComp se puede quitar.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Abilities")
	TObjectPtr<UHealthAttributeSet> HealthAttributeSet;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	// Vida con la que nace el obstaculo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle", meta=(ClampMin="0"))
	float InitialHealth = 0.f;

	UFUNCTION(BlueprintPure, Category="Obstacle")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category="Obstacle")
	float GetMaxHealth() const;

	// TODO: movido de private a public (lo llama HealthAttributeSet cuando un efecto de GAS deja al obstaculo sin vida)
	void HandleDestroyed();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grid")
	FTileCoord CurrentTile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Obstacle")
	bool bDestructible = false;

	UFUNCTION(BlueprintCallable, Category="Obstacle")
	void ApplyDamage(float Amount);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Grid")
	TObjectPtr<ABoard> Board = nullptr;

private:
	bool bDestroyHandled = false;
};