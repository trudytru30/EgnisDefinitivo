#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Characters/BoardPlayerController.h"
#include "Characters/Ally.h"
#include "CoreAndSystems/DeployManager.h"
#include "CoreAndSystems/HealthAttributeSet.h"
#include "Cards/BaseCard.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "UObject/UnrealType.h"
#include "UObject/Script.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAllySelectionInputTest, "Egnis.Input.AllySelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAllySelectionInputTest::RunTest(const FString& Parameters)
{
    UClass* ControllerClass = LoadClass<ABoardPlayerController>(nullptr,
        TEXT("/Game/BPs/Gameplay/Characters/Allies/BP_BoardPlayerController.BP_BoardPlayerController_C"));
    UClass* AllyClass = LoadClass<AAlly>(nullptr,
        TEXT("/Game/BPs/Gameplay/Characters/Allies/BP_Ally.BP_Ally_C"));
    if (!TestNotNull(TEXT("Controller Blueprint"), ControllerClass) || !TestNotNull(TEXT("Ally Blueprint"), AllyClass)) return false;

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };
    FEditorScriptExecutionGuard ScriptGuard;
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Controller = World->SpawnActor<ABoardPlayerController>(ControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Controller instance"), Controller)) return false;
    Controller->InitInputSystem();
    auto* Input = Cast<UEnhancedInputComponent>(Controller->InputComponent);
    if (!TestNotNull(TEXT("Enhanced Input component"), Input)) return false;

    auto SetObject = [&](FName Name, UObject* Object)
    {
        auto* Property = FindFProperty<FObjectPropertyBase>(ControllerClass, Name);
        if (!TestNotNull(*Name.ToString(), Property)) return;
        Property->SetObjectPropertyValue_InContainer(Controller, Object);
    };
    auto* Deployment = NewObject<UDeployManager>(Controller);
    Deployment->Initialize(AllyClass, 4);
    SetObject(TEXT("DPM"), Deployment);
    // Populate the real deployment roster without starting a battle or modifying a level.
    auto& Roster = const_cast<TArray<TPair<TWeakObjectPtr<AAlly>, FName>>&>(Deployment->GetDeployedHeroes());
    TArray<AAlly*> Allies;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        auto* Ally = World->SpawnActor<AAlly>(AllyClass, FVector(Index * 200.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Ally instance"), Ally)) return false;
        Ally->HealthAttributeSet->InitHealth(100.f);
        Allies.Add(Ally);
        Roster.Emplace(Ally, FName(*FString::Printf(TEXT("TestHero%d"), Index)));
    }
    auto Fire = [&](const TCHAR* ActionName)
    {
        UInputAction* Action = LoadObject<UInputAction>(nullptr, *FString::Printf(TEXT("/Game/Inputs/%s.%s"), ActionName, ActionName));
        if (!TestNotNull(ActionName, Action)) return;
        int32 Count = 0;
        for (const auto& Binding : Input->GetActionEventBindings())
        {
            if (Binding->GetAction() == Action)
            {
                TestTrue(TEXT("One event per press"), Binding->GetTriggerEvent() == ETriggerEvent::Started);
                Binding->Execute(FInputActionInstance(Action));
                ++Count;
            }
        }
        TestEqual(TEXT("Exactly one binding per selection action"), Count, 1);
    };
    auto Selected = [&](int32 Index)
    {
        TestTrue(TEXT("Expected selected ally"), Controller->GetSelectedAlly() == Allies[Index]);
    };
    Fire(TEXT("IA_PreviousAlly")); Selected(0);
    SetObject(TEXT("SelectedAlly"), nullptr);
    Fire(TEXT("IA_NextAlly")); Selected(0);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Fire(*FString::Printf(TEXT("IA_SelectAlly%d"), Index + 1)); Selected(Index);
        // SetOutline uses a Niagara aura; rendering must be checked in Play, not a headless commandlet.
        auto* VisualSelection = FindFProperty<FObjectPropertyBase>(ControllerClass, TEXT("PiezaSeleccionada"));
        TestTrue(TEXT("Blueprint visual selection follows keyboard"), VisualSelection && VisualSelection->GetObjectPropertyValue_InContainer(Controller) == Allies[Index]);
    }
    Fire(TEXT("IA_NextAlly")); Selected(3);
    Fire(TEXT("IA_SelectAlly1")); Fire(TEXT("IA_PreviousAlly")); Selected(0);
    // Mouse selection and keyboard navigation share SelectedAlly; no separate index can get stale.
    SetObject(TEXT("SelectedAlly"), Allies[1]);
    Fire(TEXT("IA_NextAlly")); Selected(2);
    Fire(TEXT("IA_PreviousAlly")); Selected(1);
    Allies[1]->HealthAttributeSet->InitHealth(0.f);
    Fire(TEXT("IA_SelectAlly1")); Fire(TEXT("IA_NextAlly")); Selected(2);
    Fire(TEXT("IA_SelectAlly2")); Selected(2);
    Fire(TEXT("IA_PreviousAlly")); Selected(0);
    SetObject(TEXT("PendingCard"), GetMutableDefault<UBaseCard>());
    Fire(TEXT("IA_SelectAlly4")); Selected(0);
    Fire(TEXT("IA_NextAlly")); Selected(0);
    SetObject(TEXT("PendingCard"), nullptr);
    Allies[2]->Destroy();
    Fire(TEXT("IA_NextAlly")); Selected(3);
    Roster.RemoveAt(3);
    Fire(TEXT("IA_SelectAlly1")); Fire(TEXT("IA_SelectAlly4")); Selected(0);
    Roster.Empty();
    Fire(TEXT("IA_NextAlly")); Fire(TEXT("IA_SelectAlly1")); Selected(0);
    SetObject(TEXT("DPM"), nullptr);
    Fire(TEXT("IA_PreviousAlly")); Selected(0);
    return true;
}
#endif