#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EgnisEventActions.generated.h"

/** Acciones de la columna Actions del Excel de eventos: "Comando argumento" (varias separadas por ';'). */
UCLASS()
class EGNISDEF_API UEgnisEventActions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Ejecuta una acción sobre la run actual. Devuelve true si la conocía. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Actions", meta = (WorldContext = "WorldContextObject"))
	static bool ExecuteAction(UObject* WorldContextObject, const FString& Action);
};