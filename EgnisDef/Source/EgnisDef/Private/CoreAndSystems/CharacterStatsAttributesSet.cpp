#include "CoreAndSystems/CharacterStatsAttributesSet.h"

void UCharacterStatsAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Una herida o un debuff puede bajar las stats, nunca por debajo de 0
	if (Attribute == GetPowerAttribute() || Attribute == GetPietyAttribute()
	|| Attribute == GetSpeedAttribute() || Attribute == GetRangeAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
}