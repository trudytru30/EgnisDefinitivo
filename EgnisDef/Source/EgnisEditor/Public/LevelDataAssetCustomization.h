#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class FLevelDataAssetCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
};
