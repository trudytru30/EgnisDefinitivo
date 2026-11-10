#include "Map/MapNode.h"
#include "Map/LevelMapManager.h"
#include "Components/SphereComponent.h"
#include "InputCoreTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AMapNode::AMapNode()
{
    PrimaryActorTick.bCanEverTick = false;
    ClickTarget = CreateDefaultSubobject<USphereComponent>(TEXT("ClickTarget"));
    SetRootComponent(ClickTarget);
    ClickTarget->InitSphereRadius(50.f);
    ClickTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ClickTarget->SetCollisionResponseToAllChannels(ECR_Ignore);
    ClickTarget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    ClickTarget->OnClicked.AddDynamic(this, &AMapNode::HandleClicked);
}

void AMapNode::SetManager(ALevelMapManager* NewManager)
{
    Manager = NewManager;
}

void AMapNode::UpdateState(EMapNodeState NewState)
{
    CurrentState = NewState;
    RefreshVisualState(NewState);
}

bool AMapNode::TryEnterNode()
{
    return IsValid(Manager) && Manager->TryEnterNode(Data.NodeID);
}

void AMapNode::HandleClicked(UPrimitiveComponent* Component, FKey Button)
{
    if (Button == EKeys::LeftMouseButton) TryEnterNode();
}

void AMapNode::BeginPlay()
{
    Super::BeginPlay();
    TArray<UPrimitiveComponent*> Components;
    GetComponents(Components);
    for (UPrimitiveComponent* Component : Components)
    {
        // Legacy visual meshes remain valid click targets after Blueprint migration.
        Component->OnClicked.AddUniqueDynamic(this, &AMapNode::HandleClicked);
    }
    RefreshVisualState(CurrentState);
}

void AMapNode::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    TArray<UTextRenderComponent*> Labels;
    GetComponents(Labels);
    for (UTextRenderComponent* Label : Labels)
    {
        if (Label->GetFName() == TEXT("DifLevel"))
            Label->SetText(FText::AsNumber(Data.DifficultyLevel));
    }
}

void AMapNode::RefreshVisualState_Implementation(EMapNodeState NewState)
{
    if (!NodeMaterial)
    {
        TArray<UStaticMeshComponent*> Meshes;
        GetComponents(Meshes);
        for (UStaticMeshComponent* Mesh : Meshes)
        {
            if (Mesh->GetFName() == TEXT("Sphere"))
            {
                NodeMaterial = Mesh->CreateDynamicMaterialInstance(0);
                break;
            }
        }
    }
    if (NodeMaterial)
    {
        FLinearColor Color(0.18f, 0.18f, 0.18f);
        switch (NewState)
        {
        case EMapNodeState::Available: Color = FLinearColor(0.1f, 0.8f, 0.25f); break;
        case EMapNodeState::InProgress: Color = FLinearColor(1.f, 0.65f, 0.05f); break;
        case EMapNodeState::Completed: Color = FLinearColor(0.15f, 0.4f, 0.85f); break;
        default: break;
        }
        NodeMaterial->SetVectorParameterValue(TEXT("NodeColor"), Color);
    }
}

