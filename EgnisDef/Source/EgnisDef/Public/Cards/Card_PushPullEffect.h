#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "Card_PushPullEffect.generated.h"

UENUM(BlueprintType)
enum class EForcedMoveMode : uint8
{
	Push UMETA(DisplayName = "Empujar"),
	Pull UMETA(DisplayName = "Tirar")
};

// Mueve al objetivo Distance casillas en linea recta, alejandolo o acercandolo a
// quien juega la carta. Se detiene antes si se encuentra con el borde del tablero, un obstaculo
// u otra unidad. En Pull ademas nunca pasa de quedar pegado a quien juega la carta.
// NO USA GAS: un GameplayEffect modifica atributos, aqui lo que cambia es la posicion en el Board.
UCLASS(EditInlineNew)
class EGNISDEF_API UCard_PushPullEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_PushPullEffect();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ForcedMove")
	EForcedMoveMode Mode = EForcedMoveMode::Push;

	// Casillas que se mueve al enemigo como máximo
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ForcedMove", meta = (ClampMin = "1"))
	int32 Distance = 1;

	virtual void Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy) override;
};