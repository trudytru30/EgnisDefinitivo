#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "CoreAndSystems/GameManager.h"
#include "CoreAndSystems/Board.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FRelocatedLevelSmokeTest, "Egnis.Relocation.LevelStartup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

class FCheckRelocatedLevel : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    FString Map;
    double Started = FPlatformTime::Seconds();
public:
    FCheckRelocatedLevel(FAutomationTestBase* InTest, const FString& InMap) : Test(InTest), Map(InMap) {}
    bool Update() override
    {
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (!World || !World->HasBegunPlay() || !World->GetFirstPlayerController())
        {
            if (FPlatformTime::Seconds() - Started < 30.0) return false;
            Test->AddError(TEXT("Relocated level failed to start: ") + Map);
            return true;
        }
        Test->TestNotNull(TEXT("GameInstance loaded"), World->GetGameInstance());
        AGameManager* Manager = Cast<AGameManager>(World->GetAuthGameMode());
        Test->TestNotNull(TEXT("Gameplay GameMode loaded"), Manager);
        if (Manager) Test->TestNotNull(TEXT("Battle scene manager initialized"), Manager->GetBattleSceneManager());
        int32 Boards = 0;
        for (TActorIterator<ABoard> It(World); It; ++It) ++Boards;
        if (Map.EndsWith(TEXT("EscenaPogra")))
            Test->TestTrue(TEXT("Gameplay board loaded"), Boards > 0);
        else
            Test->TestEqual(TEXT("Travel test fixture remains empty"), Boards, 0);
        return true;
    }
};

void FRelocatedLevelSmokeTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
    for (const TCHAR* Name : { TEXT("EscenaPogra"), TEXT("L_Test_A"), TEXT("L_Test_B") })
    {
        Names.Add(Name);
        Commands.Add(FString(TEXT("/Game/Maps/")) + Name);
    }
}

bool FRelocatedLevelSmokeTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(Parameters));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
    ADD_LATENT_AUTOMATION_COMMAND(FCheckRelocatedLevel(this, Parameters));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
