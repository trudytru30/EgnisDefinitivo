#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "MapTypes.generated.h"

UENUM(BlueprintType)
enum class EMapNodeType : uint8
{
    Combat, Boss, Camp, Event, Quest, Shop
};

UENUM(BlueprintType)
enum class EMapNodeState : uint8
{
    Locked, Available, InProgress, Completed
};

UENUM(BlueprintType)
enum class EMapSection : uint8 { First, Second, Third };

USTRUCT(BlueprintType)
struct EGNISDEF_API FMapNodeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    FName NodeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    EMapSection Section = EMapSection::First;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    EMapNodeType NodeType = EMapNodeType::Combat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map", meta=(ClampMin="1"))
    int32 DifficultyLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    TSoftObjectPtr<UWorld> Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map")
    TArray<FName> NextNodes;
};

