#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreAndSystems/AttributeSetMacros.h" // Macro ATTRIBUTE_ACCESSORS compartida
#include "PietyAttributeSet.generated.h"

// Piedad: solo la usan los aliados (AAlly), igual que Energy. Es un atributo GAS para que mas
// adelante los efectos de estado puedan subirla o bajarla con un GameplayEffect.
// De momento la leen las cartas que crean cosas con vida proporcional a ella (Card_CreateEffect).
UCLASS()
class EGNISDEF_API UPietyAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPietyAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Piety")
	FGameplayAttributeData Piety;
	ATTRIBUTE_ACCESSORS(UPietyAttributeSet, Piety)

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};