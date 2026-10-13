#include "Map/MapEncounterExit.h"
#include "Map/MapProgressionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

void AMapEncounterExit::BeginPlay()
{
    Super::BeginPlay();
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        EnableInput(PC);
        InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AMapEncounterExit::HandleComplete);
    }
}

bool AMapEncounterExit::CompleteAndReturn()
{
    if (ReturnMap.IsNull()) return false;
    UMapProgressionSubsystem* Progression = GetGameInstance()->GetSubsystem<UMapProgressionSubsystem>();
    if (!Progression || !Progression->CompleteCurrentNode()) return false;
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, ReturnMap);
    return true;
}

void AMapEncounterExit::HandleComplete() { CompleteAndReturn(); }

