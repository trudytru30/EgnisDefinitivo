#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Map/MapTypes.h"
#include "EventPoolDataAsset.generated.h"

USTRUCT(BlueprintType)
struct EGNISDEF_API FEventPoolEntry
{
	GENERATED_BODY()

	// Row_ID de la primera fila del evento en el Sheet (p. ej. General_FoundCorpse_01)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Pool")
	FName EventID;

	// Probabilidad relativa de salir (2 = el doble de probable que 1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Pool", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};

/** Pool de eventos de una zona. Los nodos Event sin EventID fijo sacan su evento de aquí. */
UCLASS(BlueprintType)
class EGNISDEF_API UEventPoolDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Zona a la que pertenece (se compara con la Section de los nodos del mapa)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event Pool")
	EMapSection Section = EMapSection::First;

	// Cada vez que sale un evento, su peso se multiplica por este valor para las siguientes veces
	// (1 = nunca pierde probabilidad, 0.25 = a la segunda vez tiene la cuarta parte, 0 = no se repite mientras queden otros)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event Pool", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RepeatWeightMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event Pool", meta = (TitleProperty = "EventID"))
	TArray<FEventPoolEntry> Events;
};