#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

// Delegado para notificar cambios de salud a la UI
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealthValue);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UFUNCTION(BlueprintCallable, Category="Health")
	float ApplyDelta(float Delta);

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetCurrentHealth() {return CurrentHealth;}

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetMaxHealth() {return MaxHealth;}

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetHealthRatio(){return GetCurrentHealth() / GetMaxHealth();}

	// Delegado que se dispara cuando la salud cambia
	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnHealthChanged OnHealthChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Health")
	float MaxHealth = 100;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Health")
	float CurrentHealth = 100;

	bool bDead = false;

private:
	void OnDeath();
	
};
