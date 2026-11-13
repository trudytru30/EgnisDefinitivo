#include "CoreAndSystems/HealthAttributeSet.h"
#include "GameplayEffectExtension.h" // Trae la definición completa de FGameplayEffectModCallbackData
#include "Characters/CharacterBase.h" // Para poder llamar a HandleDeath()
#include "Environment/ObstacleBase.h"
#include "Abilities/EgnisAbilityLibrary.h"
#include "CoreAndSystems/EgnisGameplayTags.h"
#include "CoreAndSystems/CombatLibrary.h"
#include "Characters/Enemy.h"
#include "CoreAndSystems/AudioManager.h"

UHealthAttributeSet::UHealthAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitShield(0.f);
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
	else if (Attribute == GetShieldAttribute())
	{
		// El escudo no baja de 0 (no tiene maximo)
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

bool UHealthAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}
	
	// Espinas: se guarda el daño tal como llega, antes de que el escudo se coma parte
	IncomingDamage = 0.f;
	if (Data.EvaluatedData.Attribute == GetHealthAttribute()
		&& Data.EvaluatedData.ModifierOp == EGameplayModOp::Additive
		&& Data.EvaluatedData.Magnitude < 0.f)
	{
		IncomingDamage = -Data.EvaluatedData.Magnitude;
	}

	// Solo el daño: un GE "Add" sobre Health con magnitud negativa (es como lo aplica
	// Card_AttackEffect con GE_CardDamage). Las curaciones (magnitud positiva) no tocan el escudo.
	if (Data.EvaluatedData.Attribute == GetHealthAttribute()
		&& Data.EvaluatedData.ModifierOp == EGameplayModOp::Additive
		&& Data.EvaluatedData.Magnitude < 0.f
		&& GetShield() > 0.f)
	{
		const float Damage = -Data.EvaluatedData.Magnitude;
		const float Absorbed = FMath::Min(GetShield(), Damage);

		SetShield(GetShield() - Absorbed);
		// Lo absorbido se le quita al daño: si el escudo lo cubre todo, queda en 0 y Health no cambia
		Data.EvaluatedData.Magnitude += Absorbed;
	}

	return true;
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

	// Obstaculos: no son ACharacterBase, pero tambien se destruyen al quedarse sin vida
	if (AObstacleBase* Obstacle = Cast<AObstacleBase>(OwnerActor))
	{
		if (Obstacle->bDestructible && GetHealth() <= 0.f)
		{
			Obstacle->HandleDestroyed();
		}
		return;
	}
	
	ACharacterBase* OwnerCharacter = Cast<ACharacterBase>(OwnerActor);
	if (!OwnerCharacter)
	{
		return;
	}

	// Ira irrefrenable: un golpe absorbido por el escudo tambien cuenta como "hacer daño"
	if (IncomingDamage > 0.f)
	{
		AActor* Hitter = Data.EffectSpec.GetContext().GetOriginalInstigator();
		if (Hitter && Hitter != OwnerCharacter)
		{
			OwnerCharacter->LastDamageInstigator = Hitter;
		}
	}
	
	if (Data.EvaluatedData.Magnitude < 0.f)
	{
		AActor* DamageInstigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
		if (DamageInstigator == OwnerCharacter)
		{
			DamageInstigator = nullptr;
		}

		const float DamageAmount = -Data.EvaluatedData.Magnitude;
		if (DamageInstigator)
		{
			OwnerCharacter->LastDamageInstigator = DamageInstigator;
		}
		UEgnisAbilityLibrary::SendEvent(OwnerCharacter, TAG_Event_Damage_Taken, DamageInstigator, OwnerCharacter, DamageAmount, nullptr, FGameplayTagContainer());
		UEgnisAbilityLibrary::SendEvent(DamageInstigator, TAG_Event_Damage_Dealt, DamageInstigator, OwnerCharacter, DamageAmount, nullptr, FGameplayTagContainer());
	}
	
	// Espinas: cuanto reflejaria el objetivo. Se calcula ya, con el objetivo todavia vivo, y se aplica al final
	ACharacterBase* ThornsAttacker = Cast<ACharacterBase>(Data.EffectSpec.GetContext().GetOriginalInstigator());
	float ThornsDamage = 0.f;
	if (ThornsAttacker && ThornsAttacker != OwnerCharacter)
	{
		ThornsDamage = UEgnisCombatLibrary::GetThornsDamage(OwnerCharacter, IncomingDamage);
	}
	// Riposte: se guarda ya si esto fue un golpe (aunque el escudo lo absorba)
	const bool bWasHit = IncomingDamage > 0.f;
	IncomingDamage = 0.f;
	
	// Sonido de golpe: suena tambien si el escudo absorbe todo el daño. Va antes de la posible muerte
	if (bWasHit)
	{
		if (UAudioManager* AM = OwnerCharacter->GetGameInstance()->GetSubsystem<UAudioManager>())
		{
			AM->PlayTrigger(Cast<AEnemy>(OwnerCharacter) ? EEgnisAudioTrigger::EnemyDamaged : EEgnisAudioTrigger::AllyDamaged,
				nullptr, OwnerCharacter);
		}
	}

	// Equivalente exacto a la comprobación "HealthComp->GetCurrentHealth() <= 0" que había
	// en LossHealth(). GetHealth() ya lee el CurrentValue post-clamp gracias a PreAttributeChange.
	if (GetHealth() <= 0.f)
	{
		// Reutiliza HandleDeath() tal cual — la guarda bDeathHandled ya evita que se ejecute
		// dos veces si el camino viejo (LossHealth) también llegó a dispararla.
		OwnerCharacter->HandleDeath();
	}
	
	// Espinas: lo ultimo, para que tambien refleje si el golpe mata al objetivo.
	// ApplyPureDamage ignora atacantes no validos y cantidades 0, y al no pasar por un GE no vuelve a reflejar.
	UEgnisCombatLibrary::ApplyPureDamage(ThornsAttacker, ThornsDamage);
	
	// Riposte: lo ultimo, para que Espinas se resuelva antes (si Espinas mata al atacante, no hay respuesta)
	if (bWasHit)
	{
		UEgnisCombatLibrary::TryRiposte(OwnerCharacter, ThornsAttacker);
	}
}