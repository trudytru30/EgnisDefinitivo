#include "Characters/BoardPlayerController.h"
#include "CoreAndSystems/AudioManager.h"
#include "Cards/BaseCard.h"
#include "CoreAndSystems/BattleManager.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "DrawDebugHelpers.h"
#include "CoreAndSystems/Board.h"
#include "Characters/CharacterBase.h"
#include "Cards/DeckManager.h"
#include "CoreAndSystems/GameManager.h"
#include "Characters/Ally.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/Ally.h"

ABoardPlayerController::ABoardPlayerController()
{
	bShowMouseCursor = true;
}

void ABoardPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);

	// Obtener sistema que gestiona el InputMappingContext
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IMCGameplay)
				Subsystem->AddMappingContext(IMCGameplay, 0);
			if (IMCUI)
				Subsystem->AddMappingContext(IMCUI, 1);	// Mayor prioridad
		}
	}

	// Referencia al GameManager
	if (AGameManager* GM = Cast<AGameManager>(GetWorld()->GetAuthGameMode()))
	{
		BM = GM->GetBattleManager();
		DeckManager = GM->GetDeckManager();
	}

	if (BM)
		BM->OnPlayerTurnStarted.AddDynamic(this, &ABoardPlayerController::BP_RefreshHandUI);
	else
		UE_LOG(LogTemp, Error, TEXT("[BoardPlayerController]: BattleManager is null"));

	if (GameHUDClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, GameHUDClass);
		if (HUDWidget)
			HUDWidget->AddToViewport();
	}
	BP_RefreshHandUI();
}

// Asignar controles
void ABoardPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput) return;

	if (!ClickAction || !PauseAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClickAction or PauseAction not found"));
		return;
	}
	EnhancedInput->BindAction(ClickAction,
		ETriggerEvent::Started,
		this,
		&ABoardPlayerController::HandleLeftClick
	);
	EnhancedInput->BindAction(
			PauseAction,
			ETriggerEvent::Started,
			this,
			&ABoardPlayerController::HandleMenu
	);
}

// Interacción con click izquierdo
void ABoardPlayerController::HandleLeftClick()
{
	if (bIsInMenu) return;

	FClickResult Result;

	const ECollisionChannel UnitChannel  = UnitTraceChannel;
	const ECollisionChannel BoardChannel  = BoardTraceChannel;

	FHitResult Hit;

	// ===== CONTROL DE ESTADOS DE JUEGO DE CARTAS =====
	// Comprobar que se esta jugando una carta (BeginPlayCard cambia el estado de None ECardSelectionState)
	if (SelectionState != ECardSelectionState::None)
	{
		// 1- Elegir unidad que juega la carta (Source)
		if (SelectionState == ECardSelectionState::SelectingUnit)
		{
			if (!TraceUnderCursor(UnitChannel, Hit)) return;

			AAlly* ClickedAlly = Cast<AAlly>(Hit.GetActor());
			if (!ClickedAlly)
			{
				UE_LOG(LogTemp, Warning, TEXT("BoardPlayerComponent: Clicked actor is not an ally"));
				return;
			}

			PendingSource = ClickedAlly;

			//Gris para asegurar arqueotipo completo
			/*
			 *si de verdad se intenta jugar con un aliado que no es del arquetipo correcto,
			 *el juego lo rechaza en el momento de elegir quién la juega
			 */
			if (PendingCard && PendingCard->GetColor() != EColorType::Grey &&ClickedAlly->GetArchetypeColor() != PendingCard->GetColor())
			{
				UE_LOG(LogTemp, Warning, TEXT("BoardPlayerController: %s no puede jugar una carta de arquetipo distinto"),*ClickedAlly->GetName());
				PendingSource = nullptr;
				return;
			}
			//
			// Jugar carta si no necesita target
			if (PendingCardTarget == ECardTarget::None || PendingCardTarget == ECardTarget::Self)
			{
				if (!PendingCard)
				{
					UE_LOG(LogTemp, Warning, TEXT("PendingCard is null"));
					return;
				}

				BM->PlayCard(PendingCard, PendingSource, nullptr, FVector::ZeroVector);
				UE_LOG(LogTemp, Warning, TEXT("PlayCard ejecutada"));
				if (DeckManager)
				{
					DeckManager->DiscardCardFromHand(PendingCard);
					UE_LOG(LogTemp, Warning, TEXT("Carta descartada de la mano"));
					UE_LOG(LogTemp, Warning, TEXT("Cartas en mano tras descartar: %d"), DeckManager->GetHand().Num());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("DeckManager NULL al descartar"));
				}

				BP_RefreshHandUI();
				// Resetear estado
				PendingCard = nullptr;
				PendingSource = nullptr;
				SelectionState = ECardSelectionState::None;
				CurrentIntent = EInputIntent::Move;
				return;
			}

			// Si necesita target, seguir con la fase 2
			SelectionState = ECardSelectionState::SelectingTarget;

			return;
		}
		// 2- Elegir objetivo de la carta (Target y Location)
		if (SelectionState == ECardSelectionState::SelectingTarget)
		{
			ACharacterBase* TargetUnit = nullptr;
			FVector TargetLocation = FVector::ZeroVector;
			UE_LOG(LogTemp, Warning, TEXT("[SelectingTarget] PendingTarget=%d"), (int32)PendingCardTarget);

			// Target Enemy
			if (PendingCardTarget == ECardTarget::Enemy)
			{
				if (!TraceUnderCursor(UnitChannel, Hit))
				{
					UE_LOG(LogTemp, Warning, TEXT("Target trace failed (UnitChannel)"));
					return;
				}

				UE_LOG(LogTemp, Warning, TEXT("Hit Actor=%s"),Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("None"));

				TargetUnit = Cast<ACharacterBase>(Hit.GetActor());
				UE_LOG(LogTemp, Warning, TEXT("Hit actor class: %s"),
					Hit.GetActor() ? *Hit.GetActor()->GetClass()->GetName() : TEXT("None"));
				if (!TargetUnit || TargetUnit->GetTeam() == 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Selected target is not an enemy."));
					return;
				}

			// Target Ally
			} else if (PendingCardTarget == ECardTarget::Ally)
			{
				if (!TraceUnderCursor(UnitChannel, Hit)) return;

				TargetUnit = Cast<ACharacterBase>(Hit.GetActor());
				if (!TargetUnit || TargetUnit->GetTeam() != 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("Selected target is not an ally."));
					return;
				}

			// Target Tile
			} else if (PendingCardTarget == ECardTarget::Tile)
			{
				if (!TraceUnderCursor(BoardChannel, Hit)) return;
				TargetLocation = Hit.ImpactPoint;
			}

			// Comprobación de que no se elige a la misma unidad
			if (TargetUnit == PendingSource)
			{
				UE_LOG(LogTemp, Warning, TEXT("Target cannot be the source"));
				return;
			}

			// Intentar jugar la carta
			BM->PlayCard(PendingCard, PendingSource, TargetUnit, TargetLocation);
			UE_LOG(LogTemp, Warning, TEXT("PlayCard ejecutada"));
			if (DeckManager)
			{
				DeckManager->DiscardCardFromHand(PendingCard);
				UE_LOG(LogTemp, Warning, TEXT("Carta descartada de la mano"));
				UE_LOG(LogTemp, Warning, TEXT("Cartas en mano tras descartar: %d"), DeckManager->GetHand().Num());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("DeckManager NULL al descartar"));
			}

			BP_RefreshHandUI();

			// Reset
			PendingCard = nullptr;
			PendingSource = nullptr;
			SelectionState = ECardSelectionState::None;
			CurrentIntent = EInputIntent::Move;

			return;
		}
	}

	if (TraceUnderCursor(UnitChannel, Hit))
	{
		if (ACharacterBase* ClickedCharacter = Cast<ACharacterBase>(Hit.GetActor()))
		{
			if (ClickedCharacter->GetTeam() == 0)
			{
				SelectedAlly = ClickedCharacter;
				if (UAudioManager* AM = GetGameInstance()->GetSubsystem<UAudioManager>()) AM->PlayElevateSound(0);
				UE_LOG(LogTemp, Log, TEXT("Selected Ally: %s"), *SelectedAlly->GetName());

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green,
				   FString::Printf(TEXT("Selected: %s"), *SelectedAlly->GetName()));
				}

				return; //para que solo seleccione
			}
		}
	}

	// Movimiento (Cuando no se juega carta)
	if (TraceUnderCursor(BoardChannel, Hit))
	{
		Result.bHit =true;
		Result.bHitBoard = true;
		Result.HitActor = Hit.GetActor();
		Result.WorldPoint = Hit.ImpactPoint;
	}

	if (Result.bHitBoard && Result.HitActor)
	{
		if (ABoard* Board = Cast<ABoard>(Result.HitActor))
		{
			FTileCoord Tile;
			if (Board->WorldPointToTile(Result.WorldPoint, Tile))
			{
				if (!SelectedAlly)
				{
					UE_LOG(LogTemp, Warning, TEXT("No ally selected. Click an ally first."));
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("No ally selected"));
					}
					return;
				}

				if (!BM)
				{
					UE_LOG(LogTemp, Warning, TEXT("BattleManager is null"));
					return;
				}

				BM->RequestMove(SelectedAlly, Tile);

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						-1, 2.0f, FColor::Yellow,
						FString::Printf(TEXT("Tile = (%d, %d)"), Tile.X, Tile.Y)
					);
				}

				// Debug visual del centro de la casilla
				const FVector Center = Board->TileToWorldCenter(Tile);
				DrawDebugSphere(GetWorld(), Center, 18.f, 12, FColor::Yellow, false, 2.0f);
			}
			else
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Click fuera del tablero"));
				}
			}
		}
		else
		{
			// No esta dando a Board o no esta en ella
			UE_LOG(LogTemp, Warning, TEXT("HitActor is not ABoard. Actor=%s"), *Result.HitActor->GetName());
		}
	}

	// Debug rápido
	if (GEngine)
	{
		const FString ModeStr = (CurrentIntent == EInputIntent::Action) ? TEXT("Action") : TEXT("Move");
		const FString HitStr =
			(!Result.bHit) ? TEXT("Nothing") :
			(Result.bHitUnit ? TEXT("Unit") : TEXT("Board"));

		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green,
			FString::Printf(TEXT("Mode=%s | Hit=%s | Actor=%s"),
				*ModeStr, *HitStr, Result.HitActor ? *Result.HitActor->GetName() : TEXT("None")));
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("IA_Click triggered"));
	}

	if (Result.bHit)
	{
		DrawDebugSphere(GetWorld(), Result.WorldPoint, 10.f, 12, FColor::Green, false, 1.0f);
	}

	
	BP_OnclickResolved(Result);
}


bool ABoardPlayerController::TraceUnderCursor(ECollisionChannel Channel, FHitResult& OutHit) const
{
	FVector WorldOrigin, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	const float TraceDistance = 200000.f;
	const FVector Start = WorldOrigin;
	const FVector End = WorldOrigin + (WorldDirection * TraceDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClickTrace), true);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);

	if (bHit)
	{
		DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 12.f, 12, FColor::Green, false, 2.0f);
	}

	return bHit;
}

// Gestionar controles cuando se abre un menu (ya sea de pausa, tutorial, etc.)
void ABoardPlayerController::OpenMenu()	// De momento solo el de pausa
{
	bIsInMenu = true;

	// Desactivar InputMappingContext
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->RemoveMappingContext(IMCGameplay);
		}
	}

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}

// Cerrar menus y activar los inputs para el juego
void ABoardPlayerController::CloseMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("CloseMenu"));
	bIsInMenu = false;

	// Activar InputMappingContext
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(IMCGameplay, 0);
		}
	}

	SetInputMode(FInputModeGameAndUI());
}

// Cambiar estado del Menu
void ABoardPlayerController::HandleMenu()
{
	bIsInMenu ? CloseMenu() : OpenMenu();
}

// Comprobar e iniciar accion de jugar carta
void ABoardPlayerController::BeginPlayCard(UBaseCard* Card)
{
	UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: BeginPlayCard llamada con carta: %s"), Card ? *Card->GetName() : TEXT("NULL"));
	if (!Card) return;

	if (!BM || !BM->IsPlayerTurn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: Cannot play card: Not player turn"));
		return;
	}

	// Deseleccionar carta al hacer seleccionarla otra vez
	if (SelectionState != ECardSelectionState::None && PendingCard == Card)
	{
		PendingCard = nullptr;
		PendingSource = nullptr;
		SelectionState = ECardSelectionState::None;
		CurrentIntent = EInputIntent::Move;
		BP_RefreshHandUI();
		return;
	}

	// Seleccionar carta (sustituye a la anterior si ya hay)
	PendingCard = Card;
	PendingSource = nullptr;
	PendingCardTarget = Card->GetTarget();
	SelectionState = ECardSelectionState::SelectingUnit;
	CurrentIntent = EInputIntent::Action; // Cambiar estado a accion (elegir unidades para jugar la carta)
	BP_RefreshHandUI();
}

// Solicitar al BattleManager terminar el turno (función llamada por UI)
void ABoardPlayerController::RequestEndTurn()
{
	if (!BM || !BM->IsPlayerTurn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: Cannot end turn: Not player turn"));
		return;
	} else if (SelectionState != ECardSelectionState::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: Cancelling card selection and ending turn..."));
		PendingCard = nullptr;
		PendingSource = nullptr;
		SelectionState = ECardSelectionState::None;
		CurrentIntent = EInputIntent::Move;
	}

	BM->EndTurn();
}
TArray<UBaseCard*> ABoardPlayerController::GetCurrentHand() const
{
	if (DeckManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: GetCurrentHand: %d cartas"), DeckManager->GetHand().Num());
		return DeckManager->GetHand();
	}

	UE_LOG(LogTemp, Warning, TEXT("[BoardPlayerController]: GetCurrentHand: DeckManager NULL"));
	return TArray<UBaseCard*>();
}
bool ABoardPlayerController::IsCardArchetypeAvailable(const UBaseCard* Card) const
{
	if (!DeckManager || !Card) return false;

	TArray<AActor*> AllyActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAlly::StaticClass(), AllyActors);

	TArray<ACharacterBase*> CharactersInPlay;
	for (AActor* Actor : AllyActors)
	{
		if (ACharacterBase* CharBase = Cast<ACharacterBase>(Actor))
		{
			CharactersInPlay.Add(CharBase);
		}
	}

	return DeckManager->IsCardArchetypeAvailable(Card, CharactersInPlay);
}