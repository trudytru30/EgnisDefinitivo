#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatLibrary.generated.h"

class ACharacterBase;
class AAlly;
class UBaseCard;

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
	
	// Daño que ignora escudo, Debilidad y Expuesto (quemadura...).
    static void ApplyPureDamage(ACharacterBase* Target, float Amount);
	
	// False si el personaje esta Aturdido (no puede jugar cartas ni actuar)
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool CanAct(const ACharacterBase* Character);
	
	// Paralizado: no puede moverse por su cuenta
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool CanMove(const ACharacterBase* Character);
	
	// True si el objetivo de la carta esta dentro del rango efectivo del lanzador
	// (rango de la carta + rango del personaje). Las cartas sin objetivo de unidad o casilla siempre devuelven true.
	// Misma distancia Manhattan que Board::GetTilesInRange, para que resaltado y validacion coincidan.
	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool IsCardTargetInRange(const UBaseCard* Card, const AAlly* Caster,
		const ACharacterBase* TargetCharacter, const FVector& Location);
};