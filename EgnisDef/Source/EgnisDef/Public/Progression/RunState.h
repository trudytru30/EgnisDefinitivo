#pragma once

#include "CoreMinimal.h"
#include "RunState.generated.h"

class UBaseCard;

//Reglas fijas de la run. Cambiar aquí si se cambian numeros
namespace EgnisRun
{
	constexpr int32 MaxWoundsPerHero = 3;
	constexpr int32 MaxRelicsPerHero = 1;//mirar md
	constexpr int32 MaxCampfire = 3;

	//Heridas posibles (Notion > Personajes > Heridas)
	//Cada herida tiene un efecto distinto y se puede tener solo una copia de cada una
	
	inline const TArray<FName>& GetAllWoundIds()
	{
		static const TArray<FName> WoundIds = {
			TEXT("Wound_Intestino"),//Perforación del intestino: baja piedad y velocidad
			TEXT("Wound_Tendon"),//Tendón partido: poder a la mitad
			TEXT("Wound_Tuetano"),//Putrefacción de tuétano: rango a la mitad
			TEXT("Wound_Cardiaca"),//Contusión cardiaca: máx. 3 cartas por combate
			TEXT("Wound_Higado"),//Desgarro de hígado: muere en 5 turnos, bonus de poder
			TEXT("Wound_Cerebral")//Licuación cerebral: juega una carta aleatoria al inicio del turno
		};
		return WoundIds;
	}
}

//Una carta del mazo de la run
//cada copia de carta tiene su propio nivel de mejora, aunque sean la misma carta
USTRUCT(BlueprintType)
struct EGNISDEF_API FDeckEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Deck")
	TObjectPtr<UBaseCard> Card = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Deck")
	int32 UpgradeLevel = 0;
};