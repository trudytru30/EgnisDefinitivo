#pragma once

#include "CoreMinimal.h"
#include "CardEffect.h"
#include "CardTarget.h"
#include "CardType.h"
#include "CoreAndSystems/ColorType.h"
#include "GameplayTagContainer.h"
#include "BaseCard.generated.h"

class UDeckManager;
class ACharacterBase;
class AAlly;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class EGNISDEF_API UBaseCard : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Card")
	FText GetCardName() const;

	UFUNCTION(BlueprintPure, Category="Card")
	ECardType GetCardType() const;

	UFUNCTION(BlueprintPure, Category="Card")
	FGameplayTag GetCult() const { return Cult; }
	
	// Carta en la que se convierte si ningun personaje en juego puede usarla.
	UFUNCTION(BlueprintPure, Category="Card")
	UBaseCard* GetConvertedCard() const { return ConvertedCard; }
	
	UFUNCTION(BlueprintPure, Category="Card")
	bool CanBePlayedBy(const AAlly* Ally) const;

	UFUNCTION(BlueprintPure, Category="Card")
	FLinearColor GetCultColor() const;

	UFUNCTION(BlueprintPure, Category="Card", meta=(DeprecatedFunction, DeprecationMessage="Usa GetCult o CanBePlayedBy"))
	EColorType GetColor() const;

	virtual void PostLoad() override;

	UFUNCTION(BlueprintPure, Category="Card")
	int32 GetCost() const;

	UFUNCTION(BlueprintPure, Category="Card")
	ECardTarget GetTarget() const;
	
	UFUNCTION(BlueprintPure, Category="Card")
	FText GetDescription() const;
	UFUNCTION(BlueprintPure, Category="Card")
	int32 GetRange() const;

	UFUNCTION(BlueprintPure, Category="Card")
	UTexture2D* GetCardImage() const;
	
	bool CanTargetTile(const ACharacterBase* Self, const FTileCoord& Tile) const;
	bool CanTargetObstacle() const;
	
#pragma region Functions
	// Acciones de las cartas
	void Execute(UDeckManager* Deck, ACharacterBase* Self, ACharacterBase* TargetCharacter, FVector Location);

	// Respuesta de Riposte: ejecuta solo los efectos contra enemigos de la carta, sobre Target.
	void ExecuteAgainst(ACharacterBase* Self, ACharacterBase* Target);

	bool HasDirectDamageEffect() const;
	
	//nuevo reunion: notificar a los efectos de esta carta que ha entrado/salido
	// de la mano sin haberse jugado. DeckManager llama a esto en DrawCard/DiscardCardFromHand.
	void NotifyHandEnter(ACharacterBase* Owner);
	void NotifyHandLeave(ACharacterBase* Owner);
#pragma endregion

protected:

#pragma region Stats
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	FText CardName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	TObjectPtr<UTexture2D> CardImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	ECardType CardType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card", meta=(Categories="Cult"))
	FGameplayTag Cult;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	TObjectPtr<UBaseCard> ConvertedCard;

	UPROPERTY()
	EColorType Color_DEPRECATED = EColorType::Grey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	int32 Cost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card")
	int32 Range = 1;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category="Card")
	TArray<UCardEffect*> Effects;

#pragma endregion
};