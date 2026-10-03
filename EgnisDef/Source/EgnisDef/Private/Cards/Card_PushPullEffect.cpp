#include "Cards/Card_PushPullEffect.h"
#include "Characters/CharacterBase.h"
#include "CoreAndSystems/Board.h"
#include "CoreAndSystems/AudioManager.h"

UCard_PushPullEffect::UCard_PushPullEffect()
{
	// Por defecto actua sobre un enemigo
	Target = ECardTarget::Enemy;
}

void UCard_PushPullEffect::Execute_Enemy(ACharacterBase* Self, ACharacterBase* Enemy)
{
	Super::Execute_Enemy(Self, Enemy);

	// IsValid y no solo != nullptr: si otro efecto de la misma carta ya ha matado al objetivo,
	// HandleDeath() lo ha destruido pero el puntero sigue sin ser null. Moverlo lo volveria a
	// a registrar como ocupante de una casilla.
	if (!IsValid(Self) || !IsValid(Enemy) || Self == Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("ForcedMove: Self o Enemy no son validos"));
		return;
	}

	ABoard* Board = Enemy->Board;
	if (!Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] ForcedMove: el objetivo no tiene Board"), *Enemy->GetName());
		return;
	}

	const FTileCoord Start = Enemy->CurrentTile;
	const int32 DX = Start.X - Self->CurrentTile.X;
	const int32 DY = Start.Y - Self->CurrentTile.Y;

	if (DX == 0 && DY == 0)
	{
		return;
	}

	// Direccion: el eje en el que mas separados estan. Si estan en diagonal exacta,
	// se mueve en diagonal. DX/DY apuntan de quien juega hacia el objetivo, asi que
	// Push va en ese sentido y Pull en el contrario.
	const int32 AbsX = FMath::Abs(DX);
	const int32 AbsY = FMath::Abs(DY);
	const int32 Sense = (Mode == EForcedMoveMode::Pull) ? -1 : 1;
	int32 DirX = 0;
	int32 DirY = 0;

	if (AbsX > AbsY)
	{
		DirX = Sense * FMath::Sign(DX);
	}
	else if (AbsY > AbsX)
	{
		DirY = Sense * FMath::Sign(DY);
	}
	else
	{
		DirX = Sense * FMath::Sign(DX);
		DirY = Sense * FMath::Sign(DY);
	}

	// En Pull, el objetivo no puede pasar de quedar pegado a quien juega la carta: si no, en un
	// objetivo no alineado acabaria pasando de largo por el lado.
	int32 MaxSteps = Distance;
	if (Mode == EForcedMoveMode::Pull)
	{
		MaxSteps = FMath::Min(Distance, FMath::Max(AbsX, AbsY) - 1);
	}

	// Avanza casilla a casilla y se queda con la ultima libre.
	// IsTileOccupied devuelve true tambien fuera del tablero, asi que esto cubre a la vez
	// el borde, los obstaculos y las unidades.
	FTileCoord Destination = Start;
	for (int32 Step = 1; Step <= MaxSteps; ++Step)
	{
		const FTileCoord Next(Start.X + DirX * Step, Start.Y + DirY * Step);
		if (Board->IsTileOccupied(Next))
		{
			break;
		}
		Destination = Next;
	}

	if (Destination == Start)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] ForcedMove: movimiento bloqueado o ya pegado, no se mueve"), *Enemy->GetName());
		return;
	}

	// Mismo camino que el movimiento normal: SetCurrentTile actualiza la ocupacion del Board y
	// CurrentTile (lo que usa la IA del enemigo). Se mantiene la Z actual para no alterar la altura.
	if (!Enemy->SetCurrentTile(Destination))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] ForcedMove: SetCurrentTile rechazo (%d,%d)"),
			*Enemy->GetName(), Destination.X, Destination.Y);
		return;
	}
	Enemy->SnapToCurrentTile(true);

	if (UAudioManager* AM = Self->GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayLandSound(0);

	UE_LOG(LogTemp, Log, TEXT("ForcedMove (%s): %s movido de (%d,%d) a (%d,%d)"),
		Mode == EForcedMoveMode::Pull ? TEXT("Pull") : TEXT("Push"),
		*Enemy->GetName(), Start.X, Start.Y, Destination.X, Destination.Y);
}