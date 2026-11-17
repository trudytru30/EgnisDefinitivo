#pragma once

#include "CoreMinimal.h"
#include "Cards/CardEffect.h"
#include "Card_IvyVFXEffect.generated.h"

class UIvyVFXSettings;
class AIvyVFXActor;

/** A que piezas cercanas al objetivo se propaga la hiedra (solo visual). */
UENUM(BlueprintType)
enum class EIvyPropagationTargets : uint8
{
	// Piezas del mismo equipo que el objetivo (si el objetivo es un enemigo, se propaga por enemigos).
	SameTeamAsTarget UMETA(DisplayName = "Mismo equipo que el objetivo"),
	// Solo enemigos de quien juega la carta.
	EnemiesOfCaster UMETA(DisplayName = "Enemigos de quien juega la carta"),
	// Solo aliados de quien juega la carta.
	AlliesOfCaster UMETA(DisplayName = "Aliados de quien juega la carta"),
	// Cualquier pieza.
	AnyPiece UMETA(DisplayName = "Cualquier pieza"),
};

/**
 * Efecto de carta que lanza el VFX de hiedra. SOLO ES VISUAL: no hace dano ni aplica estados.
 *
 * Como usarlo: en la carta, en el array Effects, anade un elemento "Card_IvyVFXEffect", elige un DataAsset
 * de IvyVFXSettings (por ejemplo DA_IvyVFX_Default) y pon su Target igual que el del efecto real de la carta:
 *   - Enemy / Ally : la hiedra va de quien juega la carta a la pieza objetivo y se enreda en ella.
 *   - Self         : la hiedra brota y se enreda en quien juega la carta.
 *   - Tile         : la hiedra va de quien juega la carta a la casilla elegida.
 * Con "Propagation Radius" mayor que 0 ademas salta de pieza en pieza por las piezas cercanas al objetivo.
 * Se puede poner en la misma carta junto a otros efectos (ataque, estado...): cada uno se ejecuta por su cuenta.
 *
 * Si el efecto real propaga por una lista de piezas propia (la que decida el gameplay), no uses el radio:
 * llama desde ese efecto a UCard_IvyVFXEffect::PlayIvyThroughPieces con esa misma lista.
 */
UCLASS(EditInlineNew, meta = (DisplayName = "Card_IvyVFXEffect"))
class EGNISDEF_API UCard_IvyVFXEffect : public UCardEffect
{
	GENERATED_BODY()

public:
	UCard_IvyVFXEffect();

	// DataAsset con todos los parametros del VFX (ritmo, colores, ramas, enredo...). Obligatorio.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "IvyVFX")
	TObjectPtr<UIvyVFXSettings> Settings;

	// Si esta activo, la hiedra sale de quien juega la carta. Si no, empieza directamente en el objetivo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "IvyVFX")
	bool bStartFromCaster = true;

	// Casillas (en distancia Manhattan, como el resto del tablero) alrededor del objetivo en las que se buscan piezas
	// a las que saltar despues. 0 = la hiedra solo va de quien juega la carta al objetivo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "IvyVFX|Propagacion", meta = (ClampMin = "0", ClampMax = "8"))
	int32 PropagationRadius = 0;

	// Maximo de piezas extra a las que salta la hiedra desde el objetivo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "IvyVFX|Propagacion",
		meta = (ClampMin = "1", ClampMax = "12", EditCondition = "PropagationRadius > 0"))
	int32 MaxPropagatedPieces = 3;

	// Que piezas cuentan para la propagacion.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "IvyVFX|Propagacion", meta = (EditCondition = "PropagationRadius > 0"))
	EIvyPropagationTargets PropagationTargets = EIvyPropagationTargets::SameTeamAsTarget;

	/**
	 * Lanza la hiedra por una lista de piezas, en orden (la primera es el origen). Util para efectos propios que ya
	 * calculan a quien propagan. Devuelve el actor de la hiedra (se destruye solo) para enganchar eventos, o null.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ivy VFX")
	static AIvyVFXActor* PlayIvyThroughPieces(ACharacterBase* WorldContext, UIvyVFXSettings* InSettings, const TArray<AActor*>& Pieces);

	virtual void Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy) override;
	virtual void Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally) override;
	virtual void Execute_Obstacle(ACharacterBase* Self, class AObstacleBase* Obstacle) override;
	virtual void Execute_Self(ACharacterBase* Source) override;
	virtual void Execute_Tile(ACharacterBase* Self, FVector Location) override;

private:
	void Play(ACharacterBase* Self, AActor* TargetActor, const FVector* TileLocation);
	void CollectPropagation(ACharacterBase* Self, AActor* TargetActor, const FVector& TargetLocation, TArray<AActor*>& InOutChain) const;
};
