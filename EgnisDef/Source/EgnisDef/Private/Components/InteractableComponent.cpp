#include "Components/InteractableComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"

UInteractableComponent::UInteractableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableComponent::SetHighlighted(bool bHighlighted)
{
    if (bIsHighlighted == bHighlighted) return;
    if (bHighlighted)
    {
        if (!HighlightMaterial || !IsValid(GetOwner())) return;
        TArray<UMeshComponent*> Meshes;
        GetOwner()->GetComponents<UMeshComponent>(Meshes);
        for (UMeshComponent* Mesh : Meshes)
        {
            PreviousOverlays.Add(Mesh, Mesh->GetOverlayMaterial());
            Mesh->SetOverlayMaterial(HighlightMaterial);
        }
    }
    else
    {
        for (const auto& Entry : PreviousOverlays)
        {
            if (IsValid(Entry.Key.Get())) Entry.Key->SetOverlayMaterial(Entry.Value.Get());
        }
        PreviousOverlays.Empty();
    }
    bIsHighlighted = bHighlighted;
}

void UInteractableComponent::Interact()
{
    OnInteract();
    if (IsValid(this) && IsValid(GetOwner())) OnInteracted.Broadcast();
}

void UInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetHighlighted(false);
    Super::EndPlay(EndPlayReason);
}
