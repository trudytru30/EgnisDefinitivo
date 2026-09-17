#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnergyComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class EGNISDEF_API UEnergyComponent : public UActorComponent
{
	GENERATED_BODY()

public:

#pragma region Functions
	UEnergyComponent();

	UFUNCTION(BlueprintCallable, Category="Points")
	int32 ApplyDelta(int32 Delta);

	UFUNCTION(BlueprintCallable, Category="Points")
	int32 ResetPoints();

	UFUNCTION(BlueprintCallable, Category="Points")
	int32 GetCurrentPoints() {return CurrentPoints;}

	UFUNCTION(BlueprintCallable, Category="Points")
	int32 GetMaxPoints() {return MaxPoints;}
#pragma endregion

protected:

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Points")
	int32 MaxPoints = 6;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Points")
	int32 CurrentPoints = 6;

	virtual void BeginPlay() override;
};