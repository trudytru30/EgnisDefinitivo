#include "CoreAndSystems/RunProgressionSubsystem.h"
#include "Characters/HeroDataAsset.h"
#include "Cards/BaseCard.h"                
#include "Progression/EgnisProfileSubsystem.h"    
#include "Engine/GameInstance.h"                 

DEFINE_LOG_CATEGORY_STATIC(LogEgnisRun, Log, All); 

void URunProgressionSubsystem::InitializeRunRoster(const TArray<UHeroDataAsset*>& AllHeroAssets)
{
	PlayerRoster.Empty();

	for (UHeroDataAsset* Asset : AllHeroAssets)
	{
		if (!Asset) continue;

		FHeroRuntimeData RuntimeData;
		RuntimeData.HeroDataAsset = Asset;
		RuntimeData.HeroId = GetHeroId(Asset); 
        
		RuntimeData.bIsUnlocked = (PlayerRoster.Num() == 0); 
		RuntimeData.CurrentHealth = 100.f;
		
		//Antes: PlayerRoster.Add(Asset->HeroStatsRow.RowName, RuntimeData);
		//la fila es la CLASE y dos héroes de la misma clase se pisaban
		PlayerRoster.Add(RuntimeData.HeroId, RuntimeData);
	}

	NotifyChanged();
}

TArray<FHeroRuntimeData> URunProgressionSubsystem::GetUnlockedHeroes() const
{
	TArray<FHeroRuntimeData> Result;
	for (const auto& Pair : PlayerRoster)
	{
		if (Pair.Value.bIsUnlocked)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

void URunProgressionSubsystem::UnlockHero(FName HeroId) //antes HeroRowName
{
	if (FHeroRuntimeData* FoundHero = PlayerRoster.Find(HeroId))
	{
		FoundHero->bIsUnlocked = true;
		NotifyChanged();
	}
}

void URunProgressionSubsystem::UpdateHeroHealth(FName HeroId, float NewHealth) //antes HeroRowName
{
	if (FHeroRuntimeData* FoundHero = PlayerRoster.Find(HeroId))
	{
		FoundHero->CurrentHealth = NewHealth;
		NotifyChanged();
	}
}

//Sistema de guardado de la run 

//Ciclo de la run

void URunProgressionSubsystem::StartNewRun(const TArray<UHeroDataAsset*>& StartingHeroes, const TArray<UBaseCard*>& StartingDeck)
{
	ResetRunData();
	bRunActive = true;

	for (UHeroDataAsset* HeroAsset : StartingHeroes)
	{
		AddHeroToRun(HeroAsset, true);
	}

	for (UBaseCard* Card : StartingDeck)
	{
		if (!Card) continue;
		FDeckEntry Entry;
		Entry.Card = Card;
		Deck.Add(Entry);
	}

	if (UEgnisProfileSubsystem* Profile = GetGameInstance()->GetSubsystem<UEgnisProfileSubsystem>())
	{
		Profile->RegisterRunStarted();
	}

	//TODO cuando MapProgressionSubsystem esté en Development, reiniciar aquí el mapa (ResetProgress)

	UE_LOG(LogEgnisRun, Log, TEXT("[Run] Nueva run: %d heroes, %d cartas."), PlayerRoster.Num(), Deck.Num());
	NotifyChanged();
}

void URunProgressionSubsystem::EndRun(bool bWon)
{
	if (!bRunActive)
	{
		UE_LOG(LogEgnisRun, Warning, TEXT("[Run] EndRun ignorado: no hay ninguna run activa."));
		return;
	}

	bRunActive = false;

	if (UEgnisProfileSubsystem* Profile = GetGameInstance()->GetSubsystem<UEgnisProfileSubsystem>())
	{
		Profile->RegisterRunEnded(bWon);
	}

	UE_LOG(LogEgnisRun, Log, TEXT("[Run] Run terminada: %s"), bWon ? TEXT("VICTORIA") : TEXT("DERROTA"));
	NotifyChanged();
}

void URunProgressionSubsystem::ResetRunData()
{
	bRunActive = false;
	Bones = 0;
	Campfire = EgnisRun::MaxCampfire;
	PlayerRoster.Empty();
	Deck.Empty();
	OwnedRelics.Empty();
}

void URunProgressionSubsystem::NotifyChanged()
{
	OnRunChanged.Broadcast();
}

//Héroes

FName URunProgressionSubsystem::GetHeroId(const UHeroDataAsset* HeroAsset)
{
	return HeroAsset ? HeroAsset->GetFName() : NAME_None;
}

FHeroRuntimeData* URunProgressionSubsystem::FindHero(FName HeroId)
{
	FHeroRuntimeData* Hero = PlayerRoster.Find(HeroId);
	if (!Hero)
	{
		UE_LOG(LogEgnisRun, Warning, TEXT("[Run] El heroe %s no esta en la caravana."), *HeroId.ToString());
	}
	return Hero;
}

void URunProgressionSubsystem::AddHeroToRun(UHeroDataAsset* HeroAsset, bool bUnlocked)
{
	const FName HeroId = GetHeroId(HeroAsset);
	if (HeroId.IsNone()) return;

	//si ya estaba, solo se actualiza el desbloqueo (no se duplica ni se le quitan las heridas)
	if (FHeroRuntimeData* Existing = PlayerRoster.Find(HeroId))
	{
		Existing->bIsUnlocked = Existing->bIsUnlocked || bUnlocked;
		NotifyChanged();
		return;
	}

	FHeroRuntimeData NewHero;
	NewHero.HeroDataAsset = HeroAsset;
	NewHero.HeroId = HeroId;
	NewHero.bIsUnlocked = bUnlocked;
	PlayerRoster.Add(HeroId, NewHero);

	UE_LOG(LogEgnisRun, Log, TEXT("[Run] Heroe anadido a la caravana: %s"), *HeroId.ToString());
	NotifyChanged();
}

bool URunProgressionSubsystem::GetHeroState(FName HeroId, FHeroRuntimeData& OutHero) const
{
	if (const FHeroRuntimeData* Hero = PlayerRoster.Find(HeroId))
	{
		OutHero = *Hero;
		return true;
	}
	OutHero = FHeroRuntimeData();
	return false;
}

//Heridas

FName URunProgressionSubsystem::AddRandomWound(FName HeroId)
{
	FHeroRuntimeData* Hero = FindHero(HeroId);
	if (!Hero) return NAME_None;

	if (Hero->Wounds.Num() >= EgnisRun::MaxWoundsPerHero)
	{
		UE_LOG(LogEgnisRun, Log, TEXT("[Run] %s ya tiene el maximo de heridas (%d)."), *HeroId.ToString(), EgnisRun::MaxWoundsPerHero);
		return NAME_None;
	}

	//solo se sortean las heridas que todavía no tiene
	TArray<FName> AvailableWounds;
	for (const FName& WoundId : EgnisRun::GetAllWoundIds())
	{
		if (!Hero->Wounds.Contains(WoundId))
		{
			AvailableWounds.Add(WoundId);
		}
	}
	if (AvailableWounds.Num() == 0) return NAME_None;

	const FName NewWound = AvailableWounds[FMath::RandRange(0, AvailableWounds.Num() - 1)];
	Hero->Wounds.Add(NewWound);

	UE_LOG(LogEgnisRun, Log, TEXT("[Run] %s recibe la herida %s (%d/%d)."),
		*HeroId.ToString(), *NewWound.ToString(), Hero->Wounds.Num(), EgnisRun::MaxWoundsPerHero);
	NotifyChanged();
	return NewWound;
}

bool URunProgressionSubsystem::HealOneWound(FName HeroId)
{
	FHeroRuntimeData* Hero = FindHero(HeroId);
	if (!Hero || Hero->Wounds.Num() == 0) return false;

	const FName HealedWound = Hero->Wounds[0];
	Hero->Wounds.RemoveAt(0);

	UE_LOG(LogEgnisRun, Log, TEXT("[Run] %s se cura de %s."), *HeroId.ToString(), *HealedWound.ToString());
	NotifyChanged();
	return true;
}

//Reliquias

void URunProgressionSubsystem::AddRelic(FName RelicId)
{
	if (RelicId.IsNone()) return;

	OwnedRelics.Add(RelicId);
	UE_LOG(LogEgnisRun, Log, TEXT("[Run] Reliquia conseguida: %s"), *RelicId.ToString());
	NotifyChanged();
}

bool URunProgressionSubsystem::EquipRelic(FName HeroId, FName RelicId)
{
	FHeroRuntimeData* Hero = FindHero(HeroId);
	if (!Hero) return false;

	if (!OwnedRelics.Contains(RelicId))
	{
		UE_LOG(LogEgnisRun, Warning, TEXT("[Run] No se puede equipar %s: no esta en la bolsa."), *RelicId.ToString());
		return false;
	}
	if (Hero->EquippedRelics.Num() >= EgnisRun::MaxRelicsPerHero)
	{
		UE_LOG(LogEgnisRun, Warning, TEXT("[Run] %s no tiene hueco para mas reliquias (max %d)."), *HeroId.ToString(), EgnisRun::MaxRelicsPerHero);
		return false;
	}

	OwnedRelics.RemoveSingle(RelicId);
	Hero->EquippedRelics.Add(RelicId);
	UE_LOG(LogEgnisRun, Log, TEXT("[Run] %s equipa %s."), *HeroId.ToString(), *RelicId.ToString());
	NotifyChanged();
	return true;
}

bool URunProgressionSubsystem::UnequipRelic(FName HeroId, FName RelicId)
{
	FHeroRuntimeData* Hero = FindHero(HeroId);
	if (!Hero || Hero->EquippedRelics.RemoveSingle(RelicId) == 0) return false;

	OwnedRelics.Add(RelicId);
	UE_LOG(LogEgnisRun, Log, TEXT("[Run] %s se quita %s (vuelve a la bolsa)."), *HeroId.ToString(), *RelicId.ToString());
	NotifyChanged();
	return true;
}

//Huesos

void URunProgressionSubsystem::AddBones(int32 Amount)
{
	if (Amount <= 0) return; //para restar se usa SpendBones

	Bones += Amount;
	UE_LOG(LogEgnisRun, Log, TEXT("[Run] +%d huesos (total %d)."), Amount, Bones);
	NotifyChanged();
}

bool URunProgressionSubsystem::SpendBones(int32 Amount)
{
	if (Amount < 0) return false;

	if (Bones < Amount)
	{
		UE_LOG(LogEgnisRun, Log, TEXT("[Run] No hay huesos suficientes: tienes %d, necesitas %d."), Bones, Amount);
		return false;
	}

	Bones -= Amount;
	UE_LOG(LogEgnisRun, Log, TEXT("[Run] -%d huesos (quedan %d)."), Amount, Bones);
	NotifyChanged();
	return true;
}

//Fogata

bool URunProgressionSubsystem::LoseCampfire()
{
	Campfire = FMath::Max(0, Campfire - 1);
	UE_LOG(LogEgnisRun, Log, TEXT("[Run] La fogata pierde una llama (%d/%d)."), Campfire, EgnisRun::MaxCampfire);
	NotifyChanged();

	if (Campfire == 0)
	{
		UE_LOG(LogEgnisRun, Log, TEXT("[Run] La fogata se ha apagado."));
		EndRun(false);
		return true;
	}
	return false;
}

void URunProgressionSubsystem::RestoreCampfire(int32 Amount)
{
	Campfire = FMath::Clamp(Campfire + Amount, 0, EgnisRun::MaxCampfire);
	UE_LOG(LogEgnisRun, Log, TEXT("[Run] Fogata: %d/%d."), Campfire, EgnisRun::MaxCampfire);
	NotifyChanged();
}

//Mazo

TArray<UBaseCard*> URunProgressionSubsystem::GetDeckCards() const
{
	TArray<UBaseCard*> Cards;
	for (const FDeckEntry& Entry : Deck)
	{
		if (Entry.Card)
		{
			Cards.Add(Entry.Card);
		}
	}
	return Cards;
}

void URunProgressionSubsystem::AddCardToDeck(UBaseCard* Card)
{
	if (!Card) return;

	FDeckEntry Entry;
	Entry.Card = Card;
	Deck.Add(Entry);

	UE_LOG(LogEgnisRun, Log, TEXT("[Run] Carta anadida al mazo: %s (%d cartas)."), *Card->GetName(), Deck.Num());
	NotifyChanged();
}

bool URunProgressionSubsystem::RemoveCardFromDeck(int32 DeckIndex)
{
	if (!Deck.IsValidIndex(DeckIndex)) return false;

	const FString CardName = Deck[DeckIndex].Card ? Deck[DeckIndex].Card->GetName() : TEXT("(nula)");
	Deck.RemoveAt(DeckIndex);

	UE_LOG(LogEgnisRun, Log, TEXT("[Run] Carta quitada del mazo: %s (%d cartas)."), *CardName, Deck.Num());
	NotifyChanged();
	return true;
}

bool URunProgressionSubsystem::UpgradeCard(int32 DeckIndex)
{
	if (!Deck.IsValidIndex(DeckIndex)) return false;

	Deck[DeckIndex].UpgradeLevel++;
	UE_LOG(LogEgnisRun, Log, TEXT("[Run] Carta %d mejorada a nivel %d."), DeckIndex, Deck[DeckIndex].UpgradeLevel);
	NotifyChanged();
	return true;
}

//Debug

FString URunProgressionSubsystem::DescribeRun() const
{
	auto JoinNames = [](const TArray<FName>& Names)
	{
		TArray<FString> Parts;
		for (const FName& Name : Names) Parts.Add(Name.ToString());
		return Parts.Num() > 0 ? FString::Join(Parts, TEXT(", ")) : FString(TEXT("(ninguna)"));
	};

	FString Text = FString::Printf(TEXT("  Run activa: %s\n  Huesos: %d\n  Fogata: %d/%d\n  Heroes (%d):"),
		bRunActive ? TEXT("si") : TEXT("no"), Bones, Campfire, EgnisRun::MaxCampfire, PlayerRoster.Num());

	for (const auto& Pair : PlayerRoster)
	{
		const FHeroRuntimeData& Hero = Pair.Value;
		Text += FString::Printf(TEXT("\n    %s [%s] heridas: %s | reliquias: %s"),
			*Pair.Key.ToString(),
			Hero.bIsUnlocked ? TEXT("desbloqueado") : TEXT("bloqueado"),
			*JoinNames(Hero.Wounds),
			*JoinNames(Hero.EquippedRelics));
	}

	TArray<FString> CardNames;
	for (const FDeckEntry& Entry : Deck)
	{
		FString CardName = Entry.Card ? Entry.Card->GetName() : TEXT("(nula)");
		if (Entry.UpgradeLevel > 0)
		{
			CardName += FString::Printf(TEXT("+%d"), Entry.UpgradeLevel);
		}
		CardNames.Add(CardName);
	}
	Text += FString::Printf(TEXT("\n  Mazo (%d): %s"), Deck.Num(),
		CardNames.Num() > 0 ? *FString::Join(CardNames, TEXT(", ")) : TEXT("(vacio)"));

	Text += FString::Printf(TEXT("\n  Reliquias en la bolsa: %s"), *JoinNames(OwnedRelics));
	return Text;
}

void URunProgressionSubsystem::LogRun() const
{
	TArray<FString> Lines;
	DescribeRun().ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		UE_LOG(LogEgnisRun, Log, TEXT("[Run] %s"), *Line);
	}
}