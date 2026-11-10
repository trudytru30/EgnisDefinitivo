#pragma once

#include "CoreMinimal.h"
#include "EgnisDialogueTypes.generated.h"

/** Una fila del Excel de diálogos (columnas Row_ID ... Text_es). */
USTRUCT(BlueprintType)
struct EGNISDEF_API FEgnisDialogueRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName RowID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextRowID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString CharacterID;

	/** Line, Choice, Random o Variant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName ParentRowID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	float Chance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString Conditions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString AudioCue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString Actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString TextEN;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString TextES;

	/** Hoja del Excel de la que viene la fila. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString SourceSheet;

	bool IsType(const TCHAR* InType) const { return Type.Equals(InType, ESearchCase::IgnoreCase); }
};

/** Opción que se le muestra al jugador. */
USTRUCT(BlueprintType)
struct EGNISDEF_API FEgnisDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName RowID;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Text;
};

/** Lo que se está mostrando ahora mismo. */
USTRUCT(BlueprintType)
struct EGNISDEF_API FEgnisDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName RowID;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Speaker;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Text;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TArray<FEgnisDialogueChoice> Choices;

	/** true si al pulsar continuar se termina el diálogo. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	bool bIsLast = false;
};
