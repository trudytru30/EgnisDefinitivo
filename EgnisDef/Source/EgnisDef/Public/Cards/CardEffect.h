#pragma once

#include "CoreMinimal.h"
#include "CardTarget.h"
#include "Characters/CharacterBase.h"
#include "CardEffect.generated.h"

class UDeckManager;


UCLASS(Blueprintable, Abstract)
class EGNISDEF_API UCardEffect : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Effect")
	ECardTarget Target = ECardTarget::None;

#pragma region Effects
	virtual void Execute_None(UDeckManager* Deck) {};
	virtual void Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally) {};
	virtual void Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy) {};
	virtual void Execute_Self(ACharacterBase* Source) {};
	//virtual void Execute_Tile(FVector Location) {};
	virtual void Execute_Tile(ACharacterBase* Self,FVector Location){};

	// Hooks para cartas/efectos que hacen algo por el simple
	// hecho de estar en la mano, sin necesidad de jugarlas
	// DeckManager los llama en DrawCard/DiscardCardFromHand — se
	// conectan cuando migremos DeckManager, aqui solo se declara el hook con cuerpo vacio por
	// defecto, igual que el resto de Execute_*, para que las cartas existentes no se vean
	// obligadas a implementarlo si no lo necesitan.
	virtual void Execute_OnHandEnter(ACharacterBase* Owner) {};
	virtual void Execute_OnHandLeave(ACharacterBase* Owner) {};
#pragma endregion
};