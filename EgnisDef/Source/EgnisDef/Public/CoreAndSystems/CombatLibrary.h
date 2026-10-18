#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatLibrary.generated.h"

class ACharacterBase;

// Reglas de combate compartidas. Los efectos de carta y las acciones de enemigos consultan aqui
// en vez de repetir la logica de estados cada uno por su lado.
UCLASS()
class EGNISDEF_API UEgnisCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Daño final de un golpe una vez aplicados los estados de atacante y objetivo.
	// Source puede ser nulo. Un golpe con daño base > 0 nunca baja de 1.
	UFUNCTION(BlueprintPure, Category = "Combat")
	static float CalculateDamage(const ACharacterBase* Source, const ACharacterBase* Target, float BaseDamage);
	
	// False si el personaje esta Aturdido (no puede jugar cartas ni actuar)
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool CanAct(const ACharacterBase* Character);
	
	// Paralizado: no puede moverse por su cuenta (empujes y tirones si le afectan)
	static bool CanMove(const ACharacterBase* Character);
};