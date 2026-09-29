#include "CoreAndSystems/HealthAttributeSet.h"
#include "GameplayEffectExtension.h" // Trae la definición completa de FGameplayEffectModCallbackData
#include "Characters/CharacterBase.h" // Para poder llamar a HandleDeath()

UHealthAttributeSet::UHealthAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
}

void UHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// Siempre se llama a Super primero, la clase base también hace comprobaciones internas
	Super::PreAttributeChange(Attribute, NewValue);

	// GetHealthAttribute() (generado por la macro) da un identificador único de "este es el
	// atributo Health de ESTE AttributeSet". La comparación es contra eso, no contra un string
	// ni un enum, así el compilador avisa si algo está mal escrito.
	if (Attribute == GetHealthAttribute())
	{
		// Equivalente exacto al FMath::Clamp(CurrentHealth+Delta, 0.0f, MaxHealth).
		// Aquí se aplica antes de guardar el valor
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Solo interesa reaccionar cuando el atributo modificado es Health — este mismo
	// PostGameplayEffectExecute se dispara para CUALQUIER atributo de este AttributeSet,
	// así que hay que filtrar cuál cambió antes de actuar.
	if (Data.EvaluatedData.Attribute != GetHealthAttribute())
	{
		return;
	}

	// AbilityActorInfo->AvatarActor es el actor "físico" al que pertenece este AttributeSet —
	// en este proyecto, siempre el propio ACharacterBase (Ally o Enemy), porque
	// InitAbilityActorInfo(this, this) se llamó con el mismo actor como owner y avatar.
	AActor* OwnerActor = Data.Target.AbilityActorInfo.IsValid()
		? Data.Target.AbilityActorInfo->AvatarActor.Get()
		: nullptr;

	ACharacterBase* OwnerCharacter = Cast<ACharacterBase>(OwnerActor);
	if (!OwnerCharacter)
	{
		return;
	}

	// Equivalente exacto a la comprobación "HealthComp->GetCurrentHealth() <= 0" que había
	// en LossHealth(). GetHealth() ya lee el CurrentValue post-clamp gracias a PreAttributeChange.
	if (GetHealth() <= 0.f)
	{
		// Reutiliza HandleDeath() tal cual — la guarda bDeathHandled ya evita que se ejecute
		// dos veces si el camino viejo (LossHealth) también llegó a dispararla.
		OwnerCharacter->HandleDeath();
	}
}