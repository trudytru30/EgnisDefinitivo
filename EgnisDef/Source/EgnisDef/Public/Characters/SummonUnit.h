#pragma once

#include "CoreMinimal.h"
#include "Characters/Ally.h"
#include "SummonUnit.generated.h"

// Invocacion: Un aliado mas que no se guarda entre partidas. Juega cartas y se mueve como un aliado,
// pero no cuenta para la victoria/derrota ni para la salud del equipo (ver AAlly::CountsForVictory).
// Lo crea Card_SummonEffect; no se despliega desde la fase de despliegue.
UCLASS()
class EGNISDEF_API ASummonUnit : public AAlly
{
	GENERATED_BODY()

public:
	// Una invocacion nunca cuenta como aliado a efectos de victoria/derrota
	virtual bool CountsForVictory() const override { return false; }

	// Vida con la que nace (piedad x multiplicador). Lo pone Card_SummonEffect ANTES de FinishSpawning.
	float HealthOverride = 0.f;

	// "Mareo de invocacion": Mientras sea true no puede jugar cartas ni moverse (ver UEgnisCombatLibrary::CanAct/CanMove).
	// BattleManager lo quita al empezar el siguiente turno del jugador.
	UFUNCTION(BlueprintPure, Category = "Summon")
	bool IsSummoningSick() const { return bSummoningSick; }

	void SetSummoningSick(bool bSick) { bSummoningSick = bSick; }

	// Pone la vida de la invocacion encima de lo que dice la tabla
	virtual void InitializeFromData(UHeroDataAsset* HeroData) override;

private:
	bool bSummoningSick = false;
};