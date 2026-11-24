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

	UFUNCTION(BlueprintPure, Category="Card")
	TArray<UCardEffect*> GetEffects() const { return Effects; }

	// Acciones de las cartas
	void Execute(UDeckManager* Deck, ACharacterBase* Self, ACharacterBase* TargetCharacter, FVector Location);

	// Respuesta de Riposte: ejecuta solo los efectos contra enemigos de la carta, sobre Target.
	void ExecuteAgainst(ACharacterBase* Self, ACharacterBase* Target);

	bool HasDirectDamageEffect() const;
	
	//nuevo reunion: notificar a los efectos de esta carta que ha entrado/salido
	// de la mano sin haberse jugado. DeckManager llama a esto en DrawCard/DiscardCardFromHand.
	void NotifyHandEnter(ACharacterBase* Owner);
	void NotifyHandLeave(ACharacterBase* Owner);
	void NotifyTurnEndInHand(UDeckManager* Deck);
	
	bool CanTargetTile(const ACharacterBase* Self, const FTileCoord& Tile) const;
	bool CanTargetCharacter(const ACharacterBase* Self, const ACharacterBase* Victim) const;
	bool CanTargetObstacle() const;
	int32 GetExtraRange(const ACharacterBase* Self) const;
	
	// Efimero: si no es la primera carta que se juega, se descarta y se sustituye por otra
	UFUNCTION(BlueprintPure, Category="Card")
	bool IsEphemeral() const { return bEphemeral; }

	// Duplicacion: la carta se juega dos veces al momento
	UFUNCTION(BlueprintPure, Category="Card")
	bool PlaysTwice() const { return bPlaysTwice; }
	
	UFUNCTION(BlueprintPure, Category = "Card")
	int32 GetEnergyOnKill() const { return EnergyOnKill; }

	UFUNCTION(BlueprintPure, Category = "Card|Shop")
	int32 GetShopValue() const { return ShopValue; }

protected:
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card|Shop", meta=(ClampMin="0", DisplayName="Valor en tienda"))
	int32 ShopValue = 0;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category="Card")
	TArray<UCardEffect*> Effects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card|Keywords", meta=(DisplayName="Efímero"))
	bool bEphemeral = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Card|Keywords", meta=(DisplayName="Duplicación"))
	bool bPlaysTwice = false;
	
	// Energia que recupera quien juega la carta por cada enemigo que muera mientras se juega (sin tope de energia
	// maxima). Cuenta cualquier baja de la carta (area, empujar a una casilla en llamas...), y con Duplicacion las
	// de las dos jugadas. 0 = no hace nada.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Keywords", meta = (ClampMin = "0", DisplayName = "Energía al matar"))
	int32 EnergyOnKill = 0;
};