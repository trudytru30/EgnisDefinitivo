#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameTextRow.generated.h"

USTRUCT(BlueprintType)
struct FGameTextRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Next_Row_ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Character_ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Parent_Row_ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Chance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Conditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Audio_Cue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Text_en;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Text_es;
};