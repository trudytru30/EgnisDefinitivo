#include "CoreAndSystems/PietyAttributeSet.h"

UPietyAttributeSet::UPietyAttributeSet()
{
	// El valor real lo pone AAlly::BeginPlay a partir de DefaultPiety (editable en BP)
	InitPiety(0.f);
}

void UPietyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetPietyAttribute())
	{
		// La piedad no baja de 0
		NewValue = FMath::Max(NewValue, 0.f);
	}
}