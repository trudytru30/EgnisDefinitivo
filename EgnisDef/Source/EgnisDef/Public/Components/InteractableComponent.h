#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

class UMaterialInterface;
class UMeshComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInteractableInteraction);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class EGNISDEF_API UInteractableComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UInteractableComponent();

    // Assign a translucent/emissive overlay material in Blueprint defaults.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Highlight")
    TObjectPtr<UMaterialInterface> HighlightMaterial;

    UFUNCTION(BlueprintCallable, Category="Interaction")
    void SetHighlighted(bool bHighlighted);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    void Interact();

    // Implement in a Blueprint subclass of this component.
    UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
    void OnInteract();

    // Bind in the owning Actor Blueprint for per-object behavior.
    UPROPERTY(BlueprintAssignable, Category="Interaction")
    FInteractableInteraction OnInteracted;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    bool bIsHighlighted = false;
    UPROPERTY(Transient)
    TMap<TObjectPtr<UMeshComponent>, TObjectPtr<UMaterialInterface>> PreviousOverlays;
};
