#include "Cards/CardDisplaySettings.h"
#include "Algo/Find.h"

UEgnisCardDisplaySettings::UEgnisCardDisplaySettings()
{
	CategoryName = TEXT("Egnis");
	SectionName = TEXT("Iconos de cartas");

	const EStatusType Beneficial[] = {
		EStatusType::Status_Taunt, EStatusType::Status_Thorns, EStatusType::Status_Riposte,
		EStatusType::Status_UnrestrainedRage, EStatusType::Status_Ecstatic, EStatusType::Status_Elusive
	};
	const UEnum* StatusEnum = StaticEnum<EStatusType>();
	for (int32 Index = 0; Index < StatusEnum->NumEnums() - 1; ++Index)
	{
		const EStatusType Status = static_cast<EStatusType>(StatusEnum->GetValueByIndex(Index));
		FCardStatusDisplay& Display = Statuses.Add(Status);
		Display.bBeneficial = Algo::Find(Beneficial, Status) != nullptr;
	}
}
