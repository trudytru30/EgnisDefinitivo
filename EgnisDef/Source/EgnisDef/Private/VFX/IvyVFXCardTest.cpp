#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Cards/BaseCard.h"
#include "Components/CapsuleComponent.h"
#include "CoreAndSystems/Board.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include "UObject/Script.h"
#include "VFX/Card_IvyVFXEffect.h"
#include "VFX/IvyVFXActor.h"
#include "VFX/IvyVFXSettings.h"

// Ejecuta cartas reales (UBaseCard::Execute) con el efecto de hiedra sobre un tablero y piezas de verdad.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIvyVFXCardTest, "Egnis.VFX.IvyCardEffect",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIvyVFXCardTest::RunTest(const FString& Parameters)
{
    UIvyVFXSettings* Settings = LoadObject<UIvyVFXSettings>(nullptr,
        TEXT("/Game/Media/Visual/FX/CultRelated/Doru/Ivy/DA_IvyVFX_Default.DA_IvyVFX_Default"));
    if (!TestNotNull(TEXT("DA_IvyVFX_Default existe"), Settings)) return false;

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
    ABoard* Board = World->SpawnActor<ABoard>(ABoard::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Tablero"), Board)) return false;

    auto Place = [&](int32 Team, int32 X, int32 Y)
    {
        ACharacterBase* Piece = World->SpawnActor<ACharacterBase>(ACharacterBase::StaticClass(),
            Board->TileToWorldCenter(FTileCoord(X, Y)), FRotator::ZeroRotator, Spawn);
        // Team es protected: se asigna por reflexion, igual que lo haria el editor
        FindFProperty<FIntProperty>(ACharacterBase::StaticClass(), TEXT("Team"))->SetPropertyValue_InContainer(Piece, Team);
        Piece->Board = Board;
        Piece->CurrentTile = FTileCoord(X, Y);
        Board->SetTileOccupant(FTileCoord(X, Y), Piece);
        return Piece;
    };

    ACharacterBase* Caster = Place(0, 1, 1);
    ACharacterBase* Ally = Place(0, 1, 3);
    ACharacterBase* Enemy = Place(1, 4, 4);
    ACharacterBase* EnemyNear = Place(1, 4, 5);        // a 1 casilla del objetivo
    ACharacterBase* EnemyTwo = Place(1, 6, 4);         // a 2 casillas del objetivo
    ACharacterBase* EnemyFar = Place(1, 7, 7);         // fuera de radio 2
    ACharacterBase* AllyNearTarget = Place(0, 3, 4);   // a 1 casilla pero de otro equipo

    // Carta nueva con un unico efecto de hiedra de ese Target
    UCard_IvyVFXEffect* Effect = nullptr;
    auto MakeCard = [&](ECardTarget Target)
    {
        UBaseCard* Card = NewObject<UBaseCard>(GetTransientPackage());
        Effect = NewObject<UCard_IvyVFXEffect>(Card);
        Effect->Settings = Settings;
        Effect->Target = Target;
        FArrayProperty* EffectsProperty = FindFProperty<FArrayProperty>(UBaseCard::StaticClass(), TEXT("Effects"));
        FScriptArrayHelper Helper(EffectsProperty, EffectsProperty->ContainerPtrToValuePtr<void>(Card));
        *reinterpret_cast<UCardEffect**>(Helper.GetRawPtr(Helper.AddValue())) = Effect;
        return Card;
    };

    auto IvyActors = [&]()
    {
        TArray<AIvyVFXActor*> Result;
        for (TActorIterator<AIvyVFXActor> It(World); It; ++It) Result.Add(*It);
        return Result;
    };
    auto Clear = [&]() { for (AIvyVFXActor* Ivy : IvyActors()) Ivy->Destroy(); };
    auto Near2D = [](const FVector& A, const FVector& B) { return FVector::Dist2D(A, B) < 1.0; };

    // Comprueba que hay exactamente UNA hiedra con ese numero de puntos y la devuelve (o null)
    auto ExpectIvy = [&](const FString& Label, int32 NodeCount) -> AIvyVFXActor*
    {
        TArray<AIvyVFXActor*> Ivies = IvyActors();
        if (!TestEqual(Label + TEXT(": una hiedra"), Ivies.Num(), 1)) return nullptr;
        TestEqual(Label + FString::Printf(TEXT(": %d puntos"), NodeCount), Ivies[0]->GetNodeCount(), NodeCount);
        return Ivies[0]->GetNodeCount() == NodeCount ? Ivies[0] : nullptr;
    };
    auto Run = [&](UBaseCard* Card, ACharacterBase* Aim, const FVector& Location)
    {
        Card->Execute(nullptr, Caster, Aim, Location);
    };

    // 1) Objetivo enemigo, sin propagacion: origen -> objetivo
    {
        UBaseCard* Card = MakeCard(ECardTarget::Enemy);
        TestTrue(TEXT("La carta pide objetivo enemigo"), Card->GetTarget() == ECardTarget::Enemy);
        Run(Card, Enemy, Enemy->GetActorLocation());
        if (AIvyVFXActor* Ivy = ExpectIvy(TEXT("Enemy"), 2))
        {
            TestTrue(TEXT("Enemy: crece"), Ivy->GetPhase() == EIvyPhase::Growing);
            TestTrue(TEXT("Enemy: parte del lanzador"), Near2D(Ivy->GetNodeLocation(0), Caster->GetActorLocation()));
            TestTrue(TEXT("Enemy: acaba en el objetivo"), Near2D(Ivy->GetNodeLocation(1), Enemy->GetActorLocation()));
            const double Floor = Enemy->GetActorLocation().Z - Enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            TestTrue(TEXT("Enemy: a ras de suelo"), FMath::Abs(Ivy->GetNodeLocation(1).Z - Floor) < 1.0);
        }
        Clear();
    }

    // 2) Carta Enemy sobre un aliado: BaseCard lo filtra y no se lanza nada
    Run(MakeCard(ECardTarget::Enemy), Ally, Ally->GetActorLocation());
    TestEqual(TEXT("Objetivo del equipo equivocado: sin hiedra"), IvyActors().Num(), 0);

    // 3) Aliado
    Run(MakeCard(ECardTarget::Ally), Ally, Ally->GetActorLocation());
    ExpectIvy(TEXT("Ally"), 2);
    Clear();

    // 4) Self: un solo punto, enredada en el propio lanzador
    Run(MakeCard(ECardTarget::Self), nullptr, FVector::ZeroVector);
    ExpectIvy(TEXT("Self"), 1);
    Clear();

    // 5) Casilla
    {
        const FVector TileCenter = Board->TileToWorldCenter(FTileCoord(5, 1));
        Run(MakeCard(ECardTarget::Tile), nullptr, TileCenter + FVector(0, 0, 500));
        if (AIvyVFXActor* Ivy = ExpectIvy(TEXT("Tile"), 2))
        {
            TestTrue(TEXT("Tile: la altura del clic no cuenta"), FMath::Abs(Ivy->GetNodeLocation(1).Z - Ivy->GetNodeLocation(0).Z) < 1.0);
            TestTrue(TEXT("Tile: en la casilla"), Near2D(Ivy->GetNodeLocation(1), TileCenter));
        }
        Clear();
    }

    // 6) Propagacion radio 2, mismo equipo que el objetivo: salta a EnemyNear y luego a EnemyTwo; ignora aliados y lejanos
    {
        UBaseCard* Card = MakeCard(ECardTarget::Enemy);
        Effect->PropagationRadius = 2;
        Effect->MaxPropagatedPieces = 5;
        Run(Card, Enemy, Enemy->GetActorLocation());
        if (AIvyVFXActor* Ivy = ExpectIvy(TEXT("Propagacion"), 4))
        {
            TestTrue(TEXT("Propagacion: primero la pieza mas cercana"), Near2D(Ivy->GetNodeLocation(2), EnemyNear->GetActorLocation()));
            TestTrue(TEXT("Propagacion: luego la siguiente"), Near2D(Ivy->GetNodeLocation(3), EnemyTwo->GetActorLocation()));
        }
        Clear();
    }

    // 7) Limite de piezas
    {
        UBaseCard* Card = MakeCard(ECardTarget::Enemy);
        Effect->PropagationRadius = 2;
        Effect->MaxPropagatedPieces = 1;
        Run(Card, Enemy, Enemy->GetActorLocation());
        ExpectIvy(TEXT("Limite"), 3);
        Clear();
    }

    // 8) Filtros: "Cualquier pieza" incluye al aliado cercano; "Aliados del lanzador" solo a el
    {
        UBaseCard* Card = MakeCard(ECardTarget::Enemy);
        Effect->PropagationRadius = 1;
        Effect->MaxPropagatedPieces = 5;
        Effect->PropagationTargets = EIvyPropagationTargets::AnyPiece;
        Run(Card, Enemy, Enemy->GetActorLocation());
        ExpectIvy(TEXT("Cualquiera"), 4);
        Clear();
        Effect->PropagationTargets = EIvyPropagationTargets::AlliesOfCaster;
        Run(Card, Enemy, Enemy->GetActorLocation());
        if (AIvyVFXActor* Ivy = ExpectIvy(TEXT("Aliados"), 3))
        {
            TestTrue(TEXT("Aliados: es el aliado cercano"), Near2D(Ivy->GetNodeLocation(2), AllyNearTarget->GetActorLocation()));
        }
        Clear();
    }

    // 9) Sin lanzador: la hiedra empieza en el objetivo
    {
        UBaseCard* Card = MakeCard(ECardTarget::Enemy);
        Effect->bStartFromCaster = false;
        Run(Card, Enemy, Enemy->GetActorLocation());
        ExpectIvy(TEXT("Sin lanzador"), 1);
        Clear();
    }

    // 10) Sin DataAsset: no hace nada ni rompe
    {
        UBaseCard* Card = MakeCard(ECardTarget::Enemy);
        Effect->Settings = nullptr;
        AddExpectedError(TEXT("no tiene DataAsset de Settings"), EAutomationExpectedErrorFlags::Contains, 1);
        Run(Card, Enemy, Enemy->GetActorLocation());
        TestEqual(TEXT("Sin settings: sin hiedra"), IvyActors().Num(), 0);
    }

    // 11) Carta con Duplicacion: se juega dos veces, dos hiedras
    {
        UBaseCard* Card = MakeCard(ECardTarget::Enemy);
        FindFProperty<FBoolProperty>(UBaseCard::StaticClass(), TEXT("bPlaysTwice"))->SetPropertyValue_InContainer(Card, true);
        Run(Card, EnemyFar, EnemyFar->GetActorLocation());
        TestEqual(TEXT("Duplicacion: dos hiedras"), IvyActors().Num(), 2);
        Clear();
    }

    // 12) La hiedra termina sola y se destruye
    Run(MakeCard(ECardTarget::Enemy), Enemy, Enemy->GetActorLocation());
    if (AIvyVFXActor* Ivy = ExpectIvy(TEXT("Ciclo"), 2))
    {
        for (int32 Frame = 0; Frame < 60 * 20 && IsValid(Ivy); ++Frame) Ivy->AdvanceEffect(1.f / 60.f);
        TestFalse(TEXT("Ciclo: el actor se autodestruye al acabar"), IsValid(Ivy));
    }
    return true;
}
#endif
