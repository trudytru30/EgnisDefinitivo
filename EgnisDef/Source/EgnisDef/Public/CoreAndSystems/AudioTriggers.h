#pragma once

#include "CoreMinimal.h"
#include "AudioTriggers.generated.h"

// Lista de momentos del juego en los que puede sonar algo.
// Cada valor aparece como una fila en el AudioDataAsset (tabla "Trigger Events"): ahi se elige el evento FMOD
// que suena en ese momento. Para anadir un momento nuevo basta con anadir un valor aqui y llamar a
// UAudioManager::PlayTrigger desde donde ocurra (el AudioDataAsset rellena la fila nueva solo).
UENUM(BlueprintType)
enum class EEgnisAudioTrigger : uint8
{
	/** Se pasa el raton por encima de una carta de la mano. No lo lanza C++: lo llama la UI desde Blueprint. */
	CardHover UMETA(DisplayName = "UI - Hover sobre carta"),
	/** Se selecciona una carta de la mano. No lo lanza C++: lo llama la UI desde Blueprint. */
	CardSelect UMETA(DisplayName = "UI - Seleccionar carta"),
	/** Se roba una carta. */
	CardDraw UMETA(DisplayName = "UI - Robar carta"),

	/** El jugador hace clic en uno de sus personajes y lo selecciona. */
	AllySelected UMETA(DisplayName = "Tablero - Aliado seleccionado"),
	/** Un aliado se mueve a una casilla con el movimiento normal del turno. */
	AllyMove UMETA(DisplayName = "Tablero - Aliado se mueve"),
	/** Un enemigo termina su movimiento (suena al llegar). */
	EnemyMove UMETA(DisplayName = "Tablero - Enemigo se mueve"),

	/** Efecto de ataque de una carta: Suena una vez por cada objetivo golpeado. */
	CardAttack UMETA(DisplayName = "Carta - Ataque"),
	/** Efecto de ataque en area: suena una vez por carta jugada. */
	CardAreaAttack UMETA(DisplayName = "Carta - Ataque en area"),
	/** Efecto de curacion de una carta. */
	CardHeal UMETA(DisplayName = "Carta - Curacion"),
	/** Efecto de escudo de una carta. */
	CardShield UMETA(DisplayName = "Carta - Escudo"),
	/** Efecto de movimiento de una carta (el personaje que juega la carta se desplaza). */
	CardMove UMETA(DisplayName = "Carta - Movimiento"),
	/** Efecto de empujar/atraer: suena cuando el objetivo termina de desplazarse. */
	CardPushPull UMETA(DisplayName = "Carta - Empujar / Atraer"),

	/** Un enemigo ataca a un aliado en su turno. */
	EnemyAttack UMETA(DisplayName = "Enemigo - Ataque")
};