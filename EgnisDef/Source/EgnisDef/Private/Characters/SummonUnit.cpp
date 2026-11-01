#include "Characters/SummonUnit.h"
#include "AbilitySystemComponent.h"

void ASummonUnit::InitializeFromData(UHeroDataAsset* HeroData)
{
	Super::InitializeFromData(HeroData);

	if (HealthOverride <= 0.f)
	{
		return;
	}
	
	//TODO
	// La vida de la invocacion manda sobre la Vitality de la tabla. Se pone en los dos sitios, igual que
	// AAlly::InitializeFromData: los campos (que CharacterBase::BeginPlay vuelca al AttributeSet) y GAS
	MaxHealth = HealthOverride;
	Health = HealthOverride;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->SetNumericAttributeBase(UHealthAttributeSet::GetMaxHealthAttribute(), HealthOverride);
		ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), HealthOverride);
	}

	UE_LOG(LogTemp, Log, TEXT("Invocacion %s: vida fijada a %.1f"), *GetName(), HealthOverride);
}