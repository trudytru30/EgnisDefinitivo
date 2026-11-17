#include "Cards/Card_StatusEffect.h"
#include "Components/StatusEffectComponent.h"
#include "CoreAndSystems/Board.h"

UCard_StatusEffect::UCard_StatusEffect()
{
	// Por defecto actua sobre un enemigo; para buffs se cambia a Ally o Self
	Target = ECardTarget::Enemy;
}

void UCard_StatusEffect::Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy)
{
	Super::Execute_Enemy(Self, Enemy);

	if (IsValid(Self) && IsValid(Enemy) && Self->GetTeam() == Enemy->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect: el objetivo Enemy es del mismo equipo"));
		return;
	}
	ApplyTo(Self, Enemy);
}

void UCard_StatusEffect::Execute_Ally(ACharacterBase* Self, ACharacterBase* Ally)
{
	Super::Execute_Ally(Self, Ally);

	if (IsValid(Self) && IsValid(Ally) && Self->GetTeam() != Ally->GetTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect: el objetivo Ally es de otro equipo"));
		return;
	}
	ApplyTo(Self, Ally);
}

void UCard_StatusEffect::Execute_Self(ACharacterBase* Source)
{
	Super::Execute_Self(Source);
	ApplyTo(Source, Source);
}

void UCard_StatusEffect::ApplyTo(ACharacterBase* Source, ACharacterBase* Receiver) const
{
	// IsValid y no solo != nullptr: otro efecto de la misma carta pudo matar ya al objetivo
	if (!IsValid(Source) || !IsValid(Receiver))
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect: Source o Receiver no son validos"));
		return;
	}

	if (UStatusEffectComponent* StatusComp = Receiver->StatusComp)
	{
		StatusComp->ApplyStatus(Status, Turns, Source, ApplicationTurn);
	}
}

void UCard_StatusEffect::Execute_Tile(ACharacterBase* Self, FVector Location)
{
	Super::Execute_Tile(Self, Location);

	if (!IsValid(Self) || !Self->Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect (area): Self o Board no son validos"));
		return;
	}

	// De momento solo Perfumar funciona en area. Para abrirlo a otros estados, borrar este bloque.
	if (Status != EStatusType::Status_Perfume)
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect (area): de momento solo Perfumar se puede aplicar en area"));
		return;
	}

	FTileCoord Center;
	if (!Self->Board->WorldPointToTile(Location, Center))
	{
		UE_LOG(LogTemp, Warning, TEXT("StatusEffect (area): la casilla esta fuera del tablero"));
		return;
	}

	TArray<FTileCoord> AreaTiles;
	Self->Board->GetTilesInRange(Center, AreaRadius, AreaTiles);
	AreaTiles.Add(Center);

	// Primero se recogen los afectados y luego se aplica, por si aplicar el estado cambiara algo del tablero
	TArray<ACharacterBase*> Affected;
	for (const FTileCoord& Tile : AreaTiles)
	{
		ACharacterBase* Character = Cast<ACharacterBase>(Self->Board->GetTileOccupant(Tile));
		if (!IsValid(Character))
		{
			continue;
		}

		const bool bSameTeam = Character->GetTeam() == Self->GetTeam();
		if ((AreaFilter == EStatusAreaFilter::Allies && !bSameTeam)
			|| (AreaFilter == EStatusAreaFilter::Enemies && bSameTeam))
		{
			continue;
		}
		Affected.AddUnique(Character);
	}

	for (ACharacterBase* Character : Affected)
	{
		ApplyTo(Self, Character);
	}

	UE_LOG(LogTemp, Log, TEXT("StatusEffect (area): centro (%d,%d), radio %d, %d personajes afectados"),
		Center.X, Center.Y, AreaRadius, Affected.Num());
}