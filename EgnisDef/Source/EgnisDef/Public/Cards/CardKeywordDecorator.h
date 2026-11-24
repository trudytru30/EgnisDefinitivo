#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "Styling/SlateBrush.h"
#include "CardKeywordDecorator.generated.h"

class UTexture2D;

// Decorador para un Rich Text Block con el texto de UCardDisplayLibrary::GetRichDescription:
// <kw id="Burn">quemadura</> pinta la palabra con el color de su palabra clave y <kwicon id="Burn"/> pone su icono.
// Colores, iconos y tamaño salen de Project Settings > Egnis > Iconos de cartas
UCLASS()
class EGNISDEF_API UCardKeywordDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	UCardKeywordDecorator(const FObjectInitializer& ObjectInitializer);

	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;

	const FSlateBrush* FindIconBrush(FName Id);

private:
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTexture2D>> Icons;

	// Punteros estables: las imágenes del texto apuntan a estos pinceles
	TMap<FName, TSharedRef<FSlateBrush>> Brushes;
};
