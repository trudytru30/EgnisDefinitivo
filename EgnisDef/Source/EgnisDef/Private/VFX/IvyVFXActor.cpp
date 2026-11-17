#include "VFX/IvyVFXActor.h"

#include "VFX/IvyVFXSettings.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ObjectSaveContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogIvyVFX, Log, All);

namespace IvyVFX
{
	const FVector Up(0.0, 0.0, 1.0);
	const float MinSproutScale = 0.001f;
	const int32 MaxPointsPerPath = 600;

	// Nombres de parametros que el efecto rellena en los materiales (si existen).
	const FName GlowColorParam(TEXT("GlowColor"));
	const FName TintAParam(TEXT("TintA"));
	const FName TintBParam(TEXT("TintB"));

	float EaseOutCubic(float X)
	{
		const float Inv = 1.f - X;
		return 1.f - Inv * Inv * Inv;
	}

	// Sube de 0 a 1 pasandose un poco de 1 antes de asentarse ("pop").
	float EaseOutBack(float X, float Overshoot)
	{
		const float C1 = 1.70158f * Overshoot;
		const float C3 = C1 + 1.f;
		const float Xm = X - 1.f;
		return 1.f + C3 * Xm * Xm * Xm + C1 * Xm * Xm;
	}

	FVector Radial(float AngleRad)
	{
		return FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.0);
	}

	FVector AnyPerpendicular(const FVector& Axis)
	{
		FVector A, B;
		Axis.FindBestAxisVectors(A, B);
		return A;
	}

	// Valor al azar dentro de un intervalo, aunque Min y Max esten al reves.
	float RandIn(FRandomStream& Rng, const FFloatInterval& Range)
	{
		return Rng.FRandRange(FMath::Min(Range.Min, Range.Max), FMath::Max(Range.Min, Range.Max));
	}

	UWorld* WorldOf(const UObject* Context)
	{
		return GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	}

	TArray<FIvyNode> ToNodes(const TArray<FVector>& Points)
	{
		TArray<FIvyNode> Nodes;
		Nodes.Reserve(Points.Num());
		for (const FVector& Point : Points)
		{
			Nodes.AddDefaulted_GetRef().Location = Point;
		}
		return Nodes;
	}

	// Actor transitorio que se destruye solo al acabar el efecto.
	AIvyVFXActor* SpawnIvy(UWorld* World, UIvyVFXSettings* Settings, const FVector& Location)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;

		AIvyVFXActor* Ivy = World->SpawnActor<AIvyVFXActor>(AIvyVFXActor::StaticClass(), FTransform(Location), Params);
		if (Ivy)
		{
			Ivy->Settings = Settings;
			Ivy->bAutoDestroy = true;
		}
		return Ivy;
	}
}

AIvyVFXActor::AIvyVFXActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(SceneRoot);

	// Todo es visual: ningun componente colisiona ni genera overlaps.
	auto SetupVisual = [this](UPrimitiveComponent* Component)
	{
		Component->SetupAttachment(SceneRoot);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->CanCharacterStepUpOn = ECB_No;
	};

	StemMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StemMesh"));
	SetupVisual(StemMesh);
	StemMesh->bUseAsyncCooking = false;

	LeafInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LeafInstances"));
	SetupVisual(LeafInstances);

	FlowerInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FlowerInstances"));
	SetupVisual(FlowerInstances);

	PreviewPoints = { FVector(0.0, 0.0, 0.0), FVector(200.0, 0.0, 0.0), FVector(300.0, 200.0, 0.0) };
}

// ============================================================================
// Lanzar el efecto
// ============================================================================

AIvyVFXActor* AIvyVFXActor::SpawnIvyThroughNodes(const UObject* WorldContextObject, UIvyVFXSettings* InSettings, const TArray<FIvyNode>& InNodes)
{
	UWorld* World = IvyVFX::WorldOf(WorldContextObject);
	if (!World || !InSettings || InNodes.Num() == 0)
	{
		UE_LOG(LogIvyVFX, Warning, TEXT("SpawnIvy: falta el mundo, el DataAsset de Settings o la lista de puntos esta vacia."));
		return nullptr;
	}

	AIvyVFXActor* Ivy = IvyVFX::SpawnIvy(World, InSettings, InNodes[0].Location);
	if (Ivy)
	{
		Ivy->PlayThroughNodes(InNodes);
	}
	return Ivy;
}

AIvyVFXActor* AIvyVFXActor::SpawnIvyThroughPoints(const UObject* WorldContextObject, UIvyVFXSettings* InSettings, const TArray<FVector>& Points)
{
	return SpawnIvyThroughNodes(WorldContextObject, InSettings, IvyVFX::ToNodes(Points));
}

AIvyVFXActor* AIvyVFXActor::SpawnIvyThroughActors(const UObject* WorldContextObject, UIvyVFXSettings* InSettings, const TArray<AActor*>& Actors)
{
	UWorld* World = IvyVFX::WorldOf(WorldContextObject);
	AActor* const* FirstActor = Actors.FindByPredicate([](const AActor* Actor) { return IsValid(Actor); });
	if (!World || !InSettings || !FirstActor)
	{
		UE_LOG(LogIvyVFX, Warning, TEXT("SpawnIvyThroughActors: falta el mundo, el DataAsset de Settings o no hay ningun actor valido."));
		return nullptr;
	}

	AIvyVFXActor* Ivy = IvyVFX::SpawnIvy(World, InSettings, MakeIvyNodeFromActor(*FirstActor, InSettings).Location);
	if (Ivy)
	{
		Ivy->PlayThroughActors(Actors);
	}
	return Ivy;
}

FIvyNode AIvyVFXActor::MakeIvyNodeFromActor(const AActor* Actor, const UIvyVFXSettings* InSettings)
{
	FIvyNode Node;
	if (!IsValid(Actor))
	{
		return Node;
	}

	FVector Origin, Extent;
	Actor->GetActorBounds(true, Origin, Extent);
	if (Extent.IsNearlyZero())
	{
		// Sin colision: probamos con todo lo visible.
		Actor->GetActorBounds(false, Origin, Extent);
	}

	const FVector ActorLocation = Actor->GetActorLocation();
	const bool bHasBounds = !Extent.IsNearlyZero();

	// XY del actor, Z de la base de sus bounds (las piezas tienen el pivote en el centro de la capsula).
	Node.Location = FVector(ActorLocation.X, ActorLocation.Y, bHasBounds ? Origin.Z - Extent.Z : ActorLocation.Z);

	if (InSettings && InSettings->bUseActorBounds && bHasBounds)
	{
		Node.Radius = FMath::Max(Extent.X, Extent.Y) * InSettings->ActorRadiusScale;
		Node.Height = 2.f * Extent.Z * InSettings->ActorHeightScale;
	}
	return Node;
}

void AIvyVFXActor::PlayThroughNodes(const TArray<FIvyNode>& InNodes)
{
	TraceIgnoredActors.Reset();
	StartEffect(InNodes);
}

void AIvyVFXActor::PlayThroughPoints(const TArray<FVector>& Points)
{
	PlayThroughNodes(IvyVFX::ToNodes(Points));
}

void AIvyVFXActor::PlayThroughActors(const TArray<AActor*>& Actors)
{
	TArray<FIvyNode> NewNodes;
	TraceIgnoredActors.Reset();
	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			NewNodes.Add(MakeIvyNodeFromActor(Actor, Settings));
			TraceIgnoredActors.Add(Actor);
		}
	}
	StartEffect(NewNodes);
}

void AIvyVFXActor::PreviewInEditor()
{
	TArray<AActor*> ValidActors;
	for (AActor* Actor : PreviewActors)
	{
		if (IsValid(Actor))
		{
			ValidActors.Add(Actor);
		}
	}

	if (ValidActors.Num() > 0)
	{
		PlayThroughActors(ValidActors);
		return;
	}

	TArray<FVector> WorldPoints;
	WorldPoints.Reserve(PreviewPoints.Num());
	const FTransform ActorTransform = GetActorTransform();
	for (const FVector& LocalPoint : PreviewPoints)
	{
		WorldPoints.Add(ActorTransform.TransformPosition(LocalPoint));
	}
	PlayThroughPoints(WorldPoints);
}

void AIvyVFXActor::BeginPlay()
{
	Super::BeginPlay();

	if (bPlayOnBeginPlay)
	{
		PreviewInEditor();
	}
}

void AIvyVFXActor::StartEffect(const TArray<FIvyNode>& InNodes)
{
	// Copia local: InNodes puede ser nuestro propio ActiveNodes (bucle) y ClearGeometry no debe pisarlo.
	const TArray<FIvyNode> NodesCopy = InNodes;

	Phase = EIvyPhase::Idle;
	ClearGeometry();

	if (!Settings)
	{
		UE_LOG(LogIvyVFX, Warning, TEXT("%s: no tiene DataAsset de Settings asignado, no se puede lanzar la hiedra."), *GetName());
		return;
	}
	if (NodesCopy.Num() == 0)
	{
		UE_LOG(LogIvyVFX, Warning, TEXT("%s: la lista de puntos esta vacia."), *GetName());
		return;
	}

	ActiveNodes = NodesCopy;
	BuildIvy();

	EffectTime = 0.f;
	NextNodeEvent = 0;
	LeadIndex = 0;
	GrowthCutoffTime = TNumericLimits<float>::Max();
	WitherBirthSpan = MaxBirthTime;
	Phase = EIvyPhase::Growing;

	if (Settings->TipFX && LeadPoints.Num() > 0)
	{
		TipFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(Settings->TipFX, SceneRoot, NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, FVector(Settings->ExtraFXScale),
			EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None, true);
		if (TipFXComponent)
		{
			TipFXComponent->SetWorldLocation(LeadPoints[0]);
		}
	}

	UpdateVisuals();
}

void AIvyVFXActor::StopAndClear()
{
	Phase = EIvyPhase::Idle;
	ClearGeometry();
}

float AIvyVFXActor::GetNodeArrivalTime(int32 NodeIndex) const
{
	if (!NodeArrivalTimes.IsValidIndex(NodeIndex))
	{
		return -1.f;
	}
	const float Rate = Settings ? FMath::Max(Settings->PlayRate, 0.05f) : 1.f;
	return NodeArrivalTimes[NodeIndex] / Rate;
}

// ============================================================================
// Generacion
// ============================================================================

void AIvyVFXActor::ClearGeometry()
{
	if (StemMesh)
	{
		StemMesh->ClearAllMeshSections();
	}
	if (LeafInstances)
	{
		LeafInstances->ClearInstances();
	}
	if (FlowerInstances)
	{
		FlowerInstances->ClearInstances();
	}
	DestroyTipFX();

	ActiveNodes.Reset();
	SurfaceList.Reset();
	Rings.Reset();
	Leaves.Reset();
	Flowers.Reset();
	NodeEvents.Reset();
	NodeArrivalTimes.Reset();
	LeadTimes.Reset();
	LeadPoints.Reset();
	StemVertices.Reset();
	StemColors.Reset();
	StemTriangles.Reset();
	StemNormals.Reset();
	StemUVs.Reset();
	NumStrands = 0;
	GrowEndTime = 0.f;
	MaxBirthTime = 0.f;
	bGeometryBuilt = false;
}

float AIvyVFXActor::ResolveNodeRadius(const FIvyNode& Node) const
{
	return FMath::Max((Node.Radius >= 0.f ? Node.Radius : Settings->WrapRadius) + Settings->WrapSurfaceOffset, 1.f);
}

float AIvyVFXActor::ResolveNodeHeight(const FIvyNode& Node) const
{
	return FMath::Max(Node.Height >= 0.f ? Node.Height : Settings->WrapHeight, 0.f);
}

int32 AIvyVFXActor::AddNodeSurface(int32 NodeIndex)
{
	const FIvyNode& Node = ActiveNodes[NodeIndex];
	FIvySurface Surface;
	Surface.bCylinder = true;
	Surface.Center = Node.Location + IvyVFX::Up * Settings->GroundOffset;
	Surface.Radius = ResolveNodeRadius(Node);
	Surface.Height = ResolveNodeHeight(Node);
	Surface.TopRadiusScale = Settings->WrapTopRadiusScale;
	return SurfaceList.Add(Surface);
}

void AIvyVFXActor::ProjectToSurface(int32 SurfaceIndex, FVector& InOutPoint, FVector& OutNormal) const
{
	const FIvySurface& Surface = SurfaceList[SurfaceIndex];

	if (Surface.bCylinder)
	{
		const FVector Rel = InOutPoint - Surface.Center;
		const float H = FMath::Clamp(static_cast<float>(Rel.Z), 0.f, Surface.Height);
		FVector RadialDir = FVector(Rel.X, Rel.Y, 0.0).GetSafeNormal();
		if (RadialDir.IsNearlyZero())
		{
			RadialDir = FVector(1.0, 0.0, 0.0);
		}
		const float HeightAlpha = Surface.Height > 1.f ? H / Surface.Height : 0.f;
		const float R = Surface.Radius * FMath::Lerp(1.f, Surface.TopRadiusScale, HeightAlpha);
		InOutPoint = Surface.Center + RadialDir * R + IvyVFX::Up * H;
		OutNormal = RadialDir;
		return;
	}

	OutNormal = IvyVFX::Up;

	const UWorld* World = GetWorld();
	if (Settings->bProjectToGround && World)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IvyGroundTrace), false, this);
		for (const AActor* Ignored : TraceIgnoredActors)
		{
			if (Ignored)
			{
				Query.AddIgnoredActor(Ignored);
			}
		}

		const FVector Offset = IvyVFX::Up * Settings->GroundTraceDistance;
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, InOutPoint + Offset, InOutPoint - Offset, Settings->GroundTraceChannel, Query))
		{
			InOutPoint.Z = Hit.ImpactPoint.Z + Settings->GroundOffset;
		}
	}
}

void AIvyVFXActor::BuildIvy()
{
	const UIvyVFXSettings& S = *Settings;
	FRandomStream Rng(S.Seed != 0 ? S.Seed : FMath::Rand());

	RingSides = FMath::Clamp(S.RadialSegments, 3, 12);
	RingCos.SetNum(RingSides + 1);
	RingSin.SetNum(RingSides + 1);
	for (int32 K = 0; K <= RingSides; ++K)
	{
		const float Angle = 2.f * UE_PI * static_cast<float>(K) / static_cast<float>(RingSides);
		RingCos[K] = FMath::Cos(Angle);
		RingSin[K] = FMath::Sin(Angle);
	}

	// La superficie 0 siempre es el suelo.
	SurfaceList.Reset();
	SurfaceList.AddDefaulted();

	const int32 NumNodes = ActiveNodes.Num();
	NodeArrivalTimes.Init(0.f, NumNodes);

	FIvyNodeEvent OriginEvent;
	OriginEvent.Time = 0.f;
	OriginEvent.NodeIndex = 0;
	NodeEvents.Add(OriginEvent);

	if (NumNodes == 1)
	{
		// Un solo punto: la hiedra brota y se enreda en esa misma pieza.
		BuildWrapOnly(0, 0.f, Rng);
	}
	else
	{
		if (S.bWrapNodes && S.bWrapFirstNode)
		{
			BuildWrapOnly(0, 0.f, Rng);
		}

		const bool bChain = S.PropagationMode == EIvyPropagationMode::Chain;
		float NextStart = 0.f;
		for (int32 Index = 1; Index < NumNodes; ++Index)
		{
			const int32 FromIndex = bChain ? Index - 1 : 0;
			float Arrival = NextStart;
			BuildLink(FromIndex, Index, bChain ? NextStart : 0.f, Rng, Arrival);

			NodeArrivalTimes[Index] = Arrival;
			FIvyNodeEvent Event;
			Event.Time = Arrival;
			Event.NodeIndex = Index;
			NodeEvents.Add(Event);

			NextStart = Arrival + S.NodePause;
		}
	}

	NodeEvents.StableSort([](const FIvyNodeEvent& A, const FIvyNodeEvent& B) { return A.Time < B.Time; });

	// Tiempos globales
	GrowEndTime = 0.f;
	MaxBirthTime = 0.f;
	const float StemSettle = FMath::Max(S.TipTaperTime, S.GlowDuration);
	for (const FIvyRing& Ring : Rings)
	{
		MaxBirthTime = FMath::Max(MaxBirthTime, Ring.Birth);
		GrowEndTime = FMath::Max(GrowEndTime, Ring.Birth + StemSettle);
	}
	for (const FIvySprout& Leaf : Leaves)
	{
		MaxBirthTime = FMath::Max(MaxBirthTime, Leaf.Birth);
		GrowEndTime = FMath::Max(GrowEndTime, Leaf.Birth + FMath::Max(S.LeafGrowDuration, S.GlowDuration));
	}
	for (const FIvySprout& Flower : Flowers)
	{
		MaxBirthTime = FMath::Max(MaxBirthTime, Flower.Birth);
		GrowEndTime = FMath::Max(GrowEndTime, Flower.Birth + FMath::Max(S.FlowerGrowDuration, S.GlowDuration));
	}
	for (const FIvyNodeEvent& Event : NodeEvents)
	{
		GrowEndTime = FMath::Max(GrowEndTime, Event.Time);
	}

	CommitGeometry();
	SetupSproutComponent(LeafInstances, S.LeafMesh, S.LeafMaterial, S.LeafColorA, S.LeafColorB, LeafMID, Leaves, Rng);
	SetupSproutComponent(FlowerInstances, S.FlowerMesh, S.FlowerMaterial, S.FlowerColorA, S.FlowerColorB, FlowerMID, Flowers, Rng);
	bGeometryBuilt = true;

	UE_LOG(LogIvyVFX, Verbose, TEXT("%s: hiedra generada con %d puntos -> %d tallos, %d anillos, %d vertices, %d hojas, %d flores. Crece en %.2fs."),
		*GetName(), NumNodes, NumStrands, Rings.Num(), StemVertices.Num(), Leaves.Num(), Flowers.Num(), GrowEndTime);
}

void AIvyVFXActor::AppendHelix(FIvyPath& Path, int32 SurfaceIndex, float StartAngle, float Direction, float StartTime) const
{
	const UIvyVFXSettings& S = *Settings;
	const FIvySurface& Surface = SurfaceList[SurfaceIndex];

	const float Turns = FMath::Max(S.WrapTurns, 0.f);
	const float AroundLength = 2.f * UE_PI * Surface.Radius * Turns;
	const float Length = FMath::Sqrt(FMath::Square(AroundLength) + FMath::Square(Surface.Height));
	if (Length < 1.f)
	{
		return;
	}

	const int32 NumSteps = FMath::Clamp(FMath::CeilToInt(Length / FMath::Max(S.SegmentLength, 2.f)), 2, IvyVFX::MaxPointsPerPath);
	for (int32 Step = 1; Step <= NumSteps; ++Step)
	{
		const float U = static_cast<float>(Step) / static_cast<float>(NumSteps);
		const float Angle = StartAngle + Direction * 2.f * UE_PI * Turns * U;
		const float R = Surface.Radius * FMath::Lerp(1.f, Surface.TopRadiusScale, U);
		const FVector RadialDir = IvyVFX::Radial(Angle);
		const FVector Point = Surface.Center + RadialDir * R + IvyVFX::Up * (Surface.Height * U);
		Path.Add(Point, RadialDir, StartTime + U * S.WrapDuration, SurfaceIndex);
	}
}

void AIvyVFXActor::BuildWrapOnly(int32 NodeIndex, float StartTime, FRandomStream& Rng)
{
	const UIvyVFXSettings& S = *Settings;
	const int32 SurfaceIndex = AddNodeSurface(NodeIndex);
	const FIvySurface Surface = SurfaceList[SurfaceIndex];

	const int32 NumStems = FMath::Clamp(S.StemsPerLink, 1, 12);
	const float AngleOffset = Rng.FRandRange(0.f, 2.f * UE_PI);
	for (int32 Stem = 0; Stem < NumStems; ++Stem)
	{
		const float Angle = AngleOffset + 2.f * UE_PI * static_cast<float>(Stem) / static_cast<float>(NumStems);
		const float Direction = (Stem % 2 == 0) ? 1.f : -1.f;
		const float Stagger = Stem == 0 ? 0.f : Rng.FRandRange(0.f, S.StemStagger);

		FIvyPath Path;
		const FVector RadialDir = IvyVFX::Radial(Angle);
		Path.Add(Surface.Center + RadialDir * Surface.Radius, RadialDir, StartTime + Stagger, SurfaceIndex);
		AppendHelix(Path, SurfaceIndex, Angle, Direction, StartTime + Stagger);

		float Length = 0.f;
		for (int32 Index = 1; Index < Path.Points.Num(); ++Index)
		{
			Length += FVector::Dist(Path.Points[Index], Path.Points[Index - 1]);
		}
		const float Speed = FMath::Max(Length / FMath::Max(S.WrapDuration, 0.01f), 1.f);
		AddStrand(Path, S.StemRadius, Speed, 0, Rng);
	}
}

void AIvyVFXActor::BuildLink(int32 FromIndex, int32 ToIndex, float StartTime, FRandomStream& Rng, float& OutArrivalTime)
{
	const UIvyVFXSettings& S = *Settings;
	const FIvyNode& From = ActiveNodes[FromIndex];
	const FIvyNode& To = ActiveNodes[ToIndex];

	const float FromRadius = ResolveNodeRadius(From);
	const float ToRadius = ResolveNodeRadius(To);

	const FVector Delta = To.Location - From.Location;
	FVector Dir2D = FVector(Delta.X, Delta.Y, 0.0).GetSafeNormal();
	if (Dir2D.IsNearlyZero())
	{
		Dir2D = FVector(1.0, 0.0, 0.0);
	}
	const FVector Right = FVector::CrossProduct(IvyVFX::Up, Dir2D);
	const float ForwardAngle = FMath::Atan2(static_cast<float>(Dir2D.Y), static_cast<float>(Dir2D.X));
	const float BackAngle = ForwardAngle + UE_PI;
	const FVector GroundLift = IvyVFX::Up * S.GroundOffset;

	const bool bWrap = S.bWrapNodes;
	const int32 WrapSurface = bWrap ? AddNodeSurface(ToIndex) : 0;

	const int32 NumStems = FMath::Clamp(S.StemsPerLink, 1, 12);
	const float SegmentLength = FMath::Max(S.SegmentLength, 2.f);
	OutArrivalTime = StartTime;

	for (int32 Stem = 0; Stem < NumStems; ++Stem)
	{
		// Bias reparte los tallos de -1 (izquierda) a +1 (derecha).
		const float Bias = NumStems > 1 ? (static_cast<float>(Stem) / static_cast<float>(NumStems - 1)) * 2.f - 1.f : 0.f;

		const FVector Start = From.Location + IvyVFX::Radial(ForwardAngle + Bias * FMath::DegreesToRadians(40.f)) * (FromRadius * 0.6f) + GroundLift;
		const float ArrivalAngle = BackAngle - Bias * FMath::DegreesToRadians(S.WrapArrivalSpread * 0.5f);
		const FVector End = To.Location + IvyVFX::Radial(ArrivalAngle) * (bWrap ? ToRadius : ToRadius * 0.3f) + GroundLift;

		const float Length = FVector::Dist(Start, End);
		if (Length < 1.f)
		{
			continue;
		}

		const float Duration = S.TimingMode == EIvyTimingMode::FixedDurationPerLink
			? FMath::Max(S.LinkDuration, 0.05f)
			: Length / FMath::Max(S.GrowthSpeed, 1.f);
		const float Stagger = Stem == 0 ? 0.f : Rng.FRandRange(0.f, S.StemStagger);
		const float StemStart = StartTime + Stagger;
		const float NoiseSeed = Rng.FRandRange(0.f, 1000.f);
		const float NoiseScale = (Length / 100.f) * S.NoiseFrequency;

		const int32 NumSteps = FMath::Clamp(FMath::CeilToInt(Length / SegmentLength), 2, IvyVFX::MaxPointsPerPath);

		FIvyPath Path;
		for (int32 Step = 0; Step <= NumSteps; ++Step)
		{
			const float T = static_cast<float>(Step) / static_cast<float>(NumSteps);
			// La envolvente vale 0 en los extremos: todos los tallos salen y llegan a su sitio exacto.
			// (el Max evita un seno minimamente negativo en T = 1, que al elevarlo daria NaN)
			const float Envelope = FMath::Pow(FMath::Max(FMath::Sin(UE_PI * T), 0.f), 0.8f);
			const float Lateral = Envelope * S.PathWidth * 0.5f * (Bias + 0.7f * FMath::PerlinNoise1D(NoiseSeed + T * NoiseScale));
			const float Vertical = Envelope * (S.ArcHeight + S.VerticalNoise * FMath::Abs(FMath::PerlinNoise1D(NoiseSeed + 371.3f + T * NoiseScale)));

			FVector Point = FMath::Lerp(Start, End, T) + Right * Lateral;
			FVector Normal;
			ProjectToSurface(0, Point, Normal);
			Point += IvyVFX::Up * Vertical;

			Path.Add(Point, Normal, StemStart + T * Duration, 0);
		}

		const float StemArrival = StemStart + Duration;

		if (Stem == 0)
		{
			OutArrivalTime = StemArrival;

			// El tallo guia marca por donde viaja el TipFX (solo si no retrocede en el tiempo).
			if (LeadTimes.Num() == 0 || Path.Times[0] >= LeadTimes.Last())
			{
				LeadTimes.Append(Path.Times);
				LeadPoints.Append(Path.Points);
			}
		}

		if (bWrap)
		{
			const float Direction = (Stem % 2 == 0) ? 1.f : -1.f;
			AppendHelix(Path, WrapSurface, ArrivalAngle, Direction, StemArrival);
		}

		AddStrand(Path, S.StemRadius, FMath::Max(Length / Duration, 1.f), 0, Rng);
	}
}

void AIvyVFXActor::GrowBranch(FIvyPath& OutPath, const FVector& Start, const FVector& Tangent, const FVector& Normal, int32 SurfaceIndex,
	float StartTime, float Speed, int32 Depth, FRandomStream& Rng) const
{
	const UIvyVFXSettings& S = *Settings;
	const FIvySurface& Surface = SurfaceList[SurfaceIndex];

	const float Length = IvyVFX::RandIn(Rng, S.BranchLength) * FMath::Pow(S.BranchLengthDepthScale, static_cast<float>(FMath::Max(Depth - 1, 0)));
	const float Side = Rng.FRand() < 0.5f ? -1.f : 1.f;
	const float Angle = FMath::DegreesToRadians(IvyVFX::RandIn(Rng, S.BranchAngle)) * Side;

	FVector SurfaceNormal = Normal.GetSafeNormal();
	if (SurfaceNormal.IsNearlyZero())
	{
		SurfaceNormal = IvyVFX::Up;
	}

	FVector Dir = (Tangent - SurfaceNormal * FVector::DotProduct(Tangent, SurfaceNormal)).GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		Dir = IvyVFX::AnyPerpendicular(SurfaceNormal);
	}
	Dir = Dir.RotateAngleAxisRad(Angle, SurfaceNormal);

	const float StepLength = FMath::Max(S.SegmentLength, 2.f);
	const int32 NumSteps = FMath::Clamp(FMath::CeilToInt(Length / StepLength), 2, 200);
	const float CurlRadPerCm = FMath::DegreesToRadians(S.BranchCurl) / 100.f;
	const float NoiseSeed = Rng.FRandRange(0.f, 1000.f);
	const float SafeSpeed = FMath::Max(Speed, 1.f);

	FVector Point = Start;
	float Time = StartTime;
	OutPath.Add(Point, SurfaceNormal, Time, SurfaceIndex);

	for (int32 Step = 1; Step <= NumSteps; ++Step)
	{
		// Rizo: giro suave e irregular, con tendencia a seguir curvandose hacia el lado contrario al tallo padre.
		const float Travelled = static_cast<float>(Step) * StepLength;
		const float Turn = CurlRadPerCm * StepLength * (0.35f * Side + FMath::PerlinNoise1D(NoiseSeed + Travelled * 0.02f));
		Dir = Dir.RotateAngleAxisRad(Turn, SurfaceNormal);

		Point += Dir * StepLength;

		if (Surface.bCylinder)
		{
			const double RelZ = Point.Z - Surface.Center.Z;
			if (RelZ < 0.0 || RelZ > Surface.Height)
			{
				break; // la rama se saldria de la pieza por arriba o por abajo
			}
		}

		ProjectToSurface(SurfaceIndex, Point, SurfaceNormal);

		Dir = (Dir - SurfaceNormal * FVector::DotProduct(Dir, SurfaceNormal)).GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			break;
		}

		Time += StepLength / SafeSpeed;
		OutPath.Add(Point, SurfaceNormal, Time, SurfaceIndex);
	}
}

void AIvyVFXActor::AddStrand(const FIvyPath& Path, float BaseRadius, float Speed, int32 Depth, FRandomStream& Rng)
{
	const UIvyVFXSettings& S = *Settings;
	const int32 Num = Path.Points.Num();
	if (Num < 2 || NumStrands >= S.MaxStrands)
	{
		return;
	}

	for (const FVector& Point : Path.Points)
	{
		if (Point.ContainsNaN())
		{
			UE_LOG(LogIvyVFX, Warning, TEXT("%s: se descarta un tallo con posiciones no validas (NaN). Revisa los puntos de entrada."), *GetName());
			return;
		}
	}

	TArray<float> Dist;
	Dist.SetNumUninitialized(Num);
	Dist[0] = 0.f;
	for (int32 Index = 1; Index < Num; ++Index)
	{
		Dist[Index] = Dist[Index - 1] + FVector::Dist(Path.Points[Index], Path.Points[Index - 1]);
	}
	const float TotalLength = Dist.Last();
	if (TotalLength < 0.5f)
	{
		return;
	}

	++NumStrands;

	const FLinearColor StrandColor = FMath::Lerp(S.StemColorA, S.StemColorB, Rng.FRand());
	const FTransform ToLocal = GetActorTransform().Inverse();
	const float UVLength = FMath::Max(S.UVLengthScale, 1.f);
	const int32 VertsPerRing = RingSides + 1;

	TArray<FVector> Tangents;
	TArray<float> Radii;
	Tangents.SetNumUninitialized(Num);
	Radii.SetNumUninitialized(Num);

	FVector PrevTangent = FVector::ZeroVector;
	FVector FrameN = FVector::ZeroVector;

	for (int32 Index = 0; Index < Num; ++Index)
	{
		FVector Tangent = (Path.Points[FMath::Min(Index + 1, Num - 1)] - Path.Points[FMath::Max(Index - 1, 0)]).GetSafeNormal();
		if (Tangent.IsNearlyZero())
		{
			Tangent = Index > 0 ? PrevTangent : IvyVFX::Up;
		}

		// Transporte paralelo del marco para que el tubo no se retuerza.
		if (Index == 0)
		{
			FrameN = Path.Normals[0];
		}
		else
		{
			FrameN = FQuat::FindBetweenNormals(PrevTangent, Tangent).RotateVector(FrameN);
		}
		FrameN = (FrameN - Tangent * FVector::DotProduct(FrameN, Tangent)).GetSafeNormal();
		if (FrameN.IsNearlyZero())
		{
			FrameN = IvyVFX::AnyPerpendicular(Tangent);
		}
		const FVector FrameB = FVector::CrossProduct(Tangent, FrameN);

		const float Alpha = Dist[Index] / TotalLength;
		float Radius = BaseRadius * FMath::Lerp(1.f, S.TipRadiusScale, FMath::Pow(Alpha, 1.5f));
		if (Index == Num - 1)
		{
			Radius *= 0.1f; // cierra la punta
		}

		Tangents[Index] = Tangent;
		Radii[Index] = Radius;
		PrevTangent = Tangent;

		FIvyRing Ring;
		Ring.Center = ToLocal.TransformPosition(Path.Points[Index]);
		Ring.AxisN = ToLocal.TransformVectorNoScale(FrameN);
		Ring.AxisB = ToLocal.TransformVectorNoScale(FrameB);
		Ring.Radius = Radius;
		Ring.Birth = Path.Times[Index];
		Ring.Color = StrandColor;
		const int32 RingIndex = Rings.Add(Ring);

		const float V = Dist[Index] / UVLength;
		for (int32 K = 0; K <= RingSides; ++K)
		{
			StemVertices.Add(Ring.Center); // nace colapsado; UpdateVisuals lo engorda
			StemNormals.Add(Ring.AxisN * RingCos[K] + Ring.AxisB * RingSin[K]);
			StemUVs.Add(FVector2D(static_cast<double>(K) / static_cast<double>(RingSides), V));
			StemColors.Add(FLinearColor(StrandColor.R, StrandColor.G, StrandColor.B, 0.f));
		}

		if (Index > 0)
		{
			const int32 Base0 = (RingIndex - 1) * VertsPerRing;
			const int32 Base1 = RingIndex * VertsPerRing;
			for (int32 K = 0; K < RingSides; ++K)
			{
				StemTriangles.Add(Base0 + K);
				StemTriangles.Add(Base1 + K);
				StemTriangles.Add(Base0 + K + 1);

				StemTriangles.Add(Base0 + K + 1);
				StemTriangles.Add(Base1 + K);
				StemTriangles.Add(Base1 + K + 1);
			}
		}
	}

	// Reparte brotes (hojas/flores) a lo largo del tallo, uno cada 100 / PerMeter cm con algo de azar.
	auto Scatter = [&](float PerMeter, TArray<FIvySprout>& Out, auto&& Place)
	{
		const float Spacing = 100.f / PerMeter;
		float NextAt = Spacing * Rng.FRandRange(0.3f, 1.f);
		for (int32 Index = 1; Index < Num; ++Index)
		{
			while (Dist[Index] >= NextAt && Out.Num() < S.MaxLeaves)
			{
				NextAt += Spacing * Rng.FRandRange(0.6f, 1.4f);
				FIvySprout Sprout;
				Place(Index, Sprout);
				Out.Add(Sprout);
			}
		}
	};

	// ---- Hojas ----
	if (S.LeafMesh && S.LeavesPerMeter > 0.f)
	{
		Scatter(S.LeavesPerMeter, Leaves, [&](int32 Index, FIvySprout& Leaf)
		{
			const FVector& Tangent = Tangents[Index];
			const FVector SurfaceNormal = Path.Normals[Index];
			FVector Sideways = FVector::CrossProduct(Tangent, SurfaceNormal).GetSafeNormal();
			if (Sideways.IsNearlyZero())
			{
				Sideways = IvyVFX::AnyPerpendicular(Tangent);
			}

			// La hoja sale hacia un lado, algo inclinada hacia la direccion de crecimiento y levantada del suelo.
			const float Side = Rng.FRand() < 0.5f ? -1.f : 1.f;
			const float Forward = FMath::DegreesToRadians(Rng.FRandRange(10.f, 50.f));
			const float Tilt = FMath::DegreesToRadians(IvyVFX::RandIn(Rng, S.LeafTilt));
			const FVector Flat = (Sideways * (Side * FMath::Cos(Forward)) + Tangent * FMath::Sin(Forward)).GetSafeNormal();
			const FVector LeafX = (Flat * FMath::Cos(Tilt) + SurfaceNormal * FMath::Sin(Tilt)).GetSafeNormal();
			FVector LeafZ = (SurfaceNormal - LeafX * FVector::DotProduct(SurfaceNormal, LeafX)).GetSafeNormal();
			if (LeafZ.IsNearlyZero())
			{
				LeafZ = IvyVFX::AnyPerpendicular(LeafX);
			}

			Leaf.WorldTransform = FTransform(
				FRotationMatrix::MakeFromXZ(LeafX, LeafZ).ToQuat(),
				Path.Points[Index] + Flat * (Radii[Index] * 0.5f) + SurfaceNormal * (Radii[Index] * 0.3f),
				FVector(IvyVFX::RandIn(Rng, S.LeafScale)));
			Leaf.Birth = Path.Times[Index] + IvyVFX::RandIn(Rng, S.LeafDelay);
		});
	}

	// ---- Flores ----
	if (S.FlowerMesh && S.FlowersPerMeter > 0.f)
	{
		Scatter(S.FlowersPerMeter, Flowers, [&](int32 Index, FIvySprout& Flower)
		{
			const FVector SurfaceNormal = Path.Normals[Index];
			// La flor mira hacia fuera de la superficie, ladeada al azar.
			const FVector Wobble = Rng.VRand() * 0.35f;
			FVector FlowerZ = (SurfaceNormal + Wobble).GetSafeNormal();
			if (FlowerZ.IsNearlyZero())
			{
				FlowerZ = SurfaceNormal;
			}
			const FVector FlowerX = FVector::CrossProduct(FlowerZ, IvyVFX::AnyPerpendicular(FlowerZ)).RotateAngleAxisRad(Rng.FRandRange(0.f, 2.f * UE_PI), FlowerZ);

			Flower.WorldTransform = FTransform(
				FRotationMatrix::MakeFromZX(FlowerZ, FlowerX).ToQuat(),
				Path.Points[Index] + SurfaceNormal * Radii[Index],
				FVector(IvyVFX::RandIn(Rng, S.FlowerScale)));
			Flower.Birth = Path.Times[Index] + IvyVFX::RandIn(Rng, S.FlowerDelay);
		});
	}

	// ---- Ramas (recursivo: cada rama puede echar las suyas hasta MaxBranchDepth) ----
	if (Depth < FMath::Clamp(S.MaxBranchDepth, 0, 4) && S.BranchesPerMeter > 0.f)
	{
		const float Spacing = 100.f / S.BranchesPerMeter;
		const float BranchSpeed = FMath::Max(Speed * S.BranchSpeedScale, 1.f);

		float NextAt = Spacing * Rng.FRandRange(0.4f, 1.2f);
		for (int32 Index = 1; Index < Num - 1; ++Index)
		{
			if (Dist[Index] < NextAt)
			{
				continue;
			}
			NextAt = Dist[Index] + Spacing * Rng.FRandRange(0.6f, 1.4f);

			if (NumStrands >= S.MaxStrands)
			{
				break;
			}

			FIvyPath BranchPath;
			GrowBranch(BranchPath, Path.Points[Index], Tangents[Index], Path.Normals[Index], Path.Surfaces[Index],
				Path.Times[Index], BranchSpeed, Depth + 1, Rng);
			AddStrand(BranchPath, FMath::Max(Radii[Index] * S.BranchRadiusScale, 0.15f), BranchSpeed, Depth + 1, Rng);
		}
	}
}

void AIvyVFXActor::CommitGeometry()
{
	const UIvyVFXSettings& S = *Settings;

	StemMesh->ClearAllMeshSections();
	if (StemVertices.Num() == 0 || StemTriangles.Num() == 0)
	{
		return;
	}

	// bSRGBConversion = false: el material recibe en Vertex Color exactamente el color lineal del DataAsset.
	StemMesh->CreateMeshSection_LinearColor(0, StemVertices, StemTriangles, StemNormals, StemUVs, StemColors,
		TArray<FProcMeshTangent>(), false, false);
	StemMesh->SetCastShadow(S.bCastShadows);

	StemMID = S.StemMaterial ? UMaterialInstanceDynamic::Create(S.StemMaterial, this) : nullptr;
	if (StemMID)
	{
		StemMID->SetVectorParameterValue(IvyVFX::GlowColorParam, S.GlowColor * S.GlowIntensity);
		StemMesh->SetMaterial(0, StemMID);
	}
}

void AIvyVFXActor::SetupSproutComponent(UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, UMaterialInterface* Material,
	const FLinearColor& TintA, const FLinearColor& TintB, TObjectPtr<UMaterialInstanceDynamic>& OutMID,
	TArray<FIvySprout>& Sprouts, FRandomStream& Rng)
{
	Component->ClearInstances();
	OutMID = nullptr;

	if (!Mesh || Sprouts.Num() == 0)
	{
		Sprouts.Reset();
		return;
	}

	const UIvyVFXSettings& S = *Settings;

	Component->SetStaticMesh(Mesh);
	Component->SetNumCustomDataFloats(2);
	Component->SetCastShadow(S.bCastShadows);

	UMaterialInterface* BaseMaterial = Material ? Material : Mesh->GetMaterial(0);
	if (BaseMaterial)
	{
		OutMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		OutMID->SetVectorParameterValue(IvyVFX::TintAParam, TintA);
		OutMID->SetVectorParameterValue(IvyVFX::TintBParam, TintB);
		OutMID->SetVectorParameterValue(IvyVFX::GlowColorParam, S.GlowColor * S.GlowIntensity);
		Component->SetMaterial(0, OutMID);
	}

	TArray<FTransform> Transforms;
	Transforms.Reserve(Sprouts.Num());
	for (const FIvySprout& Sprout : Sprouts)
	{
		FTransform Hidden = Sprout.WorldTransform;
		Hidden.SetScale3D(FVector(IvyVFX::MinSproutScale));
		Transforms.Add(Hidden);
	}
	Component->AddInstances(Transforms, false, true);

	for (int32 Index = 0; Index < Sprouts.Num(); ++Index)
	{
		Component->SetCustomDataValue(Index, 0, 0.f, false);          // brillo
		Component->SetCustomDataValue(Index, 1, Rng.FRand(), false);  // azar por instancia
		Sprouts[Index].LastScale = IvyVFX::MinSproutScale;
		Sprouts[Index].LastGlow = 0.f;
	}
	Component->MarkRenderStateDirty();
}

// ============================================================================
// Animacion
// ============================================================================

void AIvyVFXActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceEffect(DeltaSeconds);
}

bool AIvyVFXActor::ShouldTickIfViewportsOnly() const
{
	return Phase != EIvyPhase::Idle && IsEditorPreviewWorld();
}

bool AIvyVFXActor::IsEditorPreviewWorld() const
{
	const UWorld* World = GetWorld();
	return World && (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview);
}

void AIvyVFXActor::PreSave(FObjectPreSaveContext SaveContext)
{
	// Que una previsualizacion en el editor no se quede guardada dentro del nivel.
	if (Phase != EIvyPhase::Idle && IsEditorPreviewWorld())
	{
		StopAndClear();
	}
	Super::PreSave(SaveContext);
}

void AIvyVFXActor::AdvanceEffect(float DeltaSeconds)
{
	if (Phase == EIvyPhase::Idle || !Settings || !bGeometryBuilt)
	{
		return;
	}

	EffectTime += FMath::Max(DeltaSeconds, 0.f) * FMath::Max(Settings->PlayRate, 0.05f);

	// Los listeners de los eventos pueden parar o destruir el efecto: se comprueba tras cada aviso.
	const auto Stopped = [this]() { return !IsValid(this) || Phase == EIvyPhase::Idle || !Settings; };

	// Avisos de llegada a cada pieza.
	while (Phase == EIvyPhase::Growing && NextNodeEvent < NodeEvents.Num() && EffectTime >= NodeEvents[NextNodeEvent].Time)
	{
		const int32 NodeIndex = NodeEvents[NextNodeEvent].NodeIndex;
		++NextNodeEvent;

		if (!ActiveNodes.IsValidIndex(NodeIndex))
		{
			continue;
		}
		const FVector NodeLocation = ActiveNodes[NodeIndex].Location;

		if (NodeHasIvy(NodeIndex))
		{
			SpawnNodeFX(Settings->NodeReachedFX, NodeIndex);
		}

		OnNodeReached.Broadcast(NodeIndex, NodeLocation);
		if (Stopped())
		{
			return;
		}
	}

	if (Phase == EIvyPhase::Growing && EffectTime >= GrowEndTime && NextNodeEvent >= NodeEvents.Num())
	{
		Phase = EIvyPhase::Holding;
		HoldEndTime = EffectTime + Settings->HoldDuration;
		UpdateVisuals();
		if (TipFXComponent)
		{
			TipFXComponent->Deactivate();
		}

		OnGrowthFinished.Broadcast();
		if (Stopped())
		{
			return;
		}
	}

	if (Phase == EIvyPhase::Holding && !Settings->bStayUntilWither && EffectTime >= HoldEndTime)
	{
		Wither();
		if (Stopped())
		{
			return;
		}
	}

	if (Phase == EIvyPhase::Withering)
	{
		const float WaveDuration = Settings->WitherMode == EIvyWitherMode::AllAtOnce ? 0.f : Settings->WitherDuration;
		if (EffectTime >= WitherStartTime + WaveDuration + FMath::Max(Settings->WitherFade, 0.01f))
		{
			FinishEffect();
			return;
		}
	}

	if (Phase == EIvyPhase::Growing || Phase == EIvyPhase::Withering)
	{
		UpdateVisuals();
	}
}

float AIvyVFXActor::WitherFactor(float Birth) const
{
	if (Phase != EIvyPhase::Withering)
	{
		return 1.f;
	}

	const float Normalized = WitherBirthSpan > KINDA_SMALL_NUMBER ? FMath::Clamp(Birth / WitherBirthSpan, 0.f, 1.f) : 0.f;

	float WaveOffset = 0.f;
	switch (Settings->WitherMode)
	{
	case EIvyWitherMode::RetractToOrigin: WaveOffset = 1.f - Normalized; break;
	case EIvyWitherMode::FollowGrowth:    WaveOffset = Normalized; break;
	case EIvyWitherMode::AllAtOnce:       WaveOffset = 0.f; break;
	}

	const float LocalStart = WitherStartTime + WaveOffset * Settings->WitherDuration;
	const float X = FMath::Clamp((EffectTime - LocalStart) / FMath::Max(Settings->WitherFade, 0.01f), 0.f, 1.f);
	return 1.f - X * X * (3.f - 2.f * X);
}

void AIvyVFXActor::UpdateVisuals()
{
	if (!Settings || !bGeometryBuilt)
	{
		return;
	}
	const UIvyVFXSettings& S = *Settings;

	// ---- Tallos: cada anillo engorda cuando le pasa la punta y encoge al marchitarse ----
	const int32 VertsPerRing = RingSides + 1;
	if (Rings.Num() > 0 && StemVertices.Num() == Rings.Num() * VertsPerRing)
	{
		const float TaperTime = FMath::Max(S.TipTaperTime, 0.01f);
		const float GlowTime = FMath::Max(S.GlowDuration, 0.01f);

		for (int32 RingIndex = 0; RingIndex < Rings.Num(); ++RingIndex)
		{
			const FIvyRing& Ring = Rings[RingIndex];

			float Radius = 0.f;
			float Glow = 0.f;
			if (Ring.Birth <= GrowthCutoffTime && EffectTime >= Ring.Birth)
			{
				const float Age = EffectTime - Ring.Birth;
				Radius = Ring.Radius * IvyVFX::EaseOutCubic(FMath::Clamp(Age / TaperTime, 0.f, 1.f)) * WitherFactor(Ring.Birth);
				Glow = 1.f - FMath::Clamp(Age / GlowTime, 0.f, 1.f);
			}

			const int32 Base = RingIndex * VertsPerRing;
			for (int32 K = 0; K <= RingSides; ++K)
			{
				StemVertices[Base + K] = Ring.Center + (Ring.AxisN * RingCos[K] + Ring.AxisB * RingSin[K]) * Radius;
				StemColors[Base + K].A = Glow;
			}
		}

		static const TArray<FVector> NoNormals;
		static const TArray<FVector2D> NoUVs;
		static const TArray<FProcMeshTangent> NoTangents;
		StemMesh->UpdateMeshSection_LinearColor(0, StemVertices, NoNormals, NoUVs, StemColors, NoTangents, false);
	}

	UpdateSprouts(LeafInstances, Leaves, S.LeafGrowDuration, S.LeafOvershoot);
	UpdateSprouts(FlowerInstances, Flowers, S.FlowerGrowDuration, S.FlowerOvershoot);
	UpdateTipFX();
}

void AIvyVFXActor::UpdateSprouts(UInstancedStaticMeshComponent* Component, TArray<FIvySprout>& Sprouts, float GrowDuration, float Overshoot)
{
	if (Sprouts.Num() == 0 || Component->GetInstanceCount() != Sprouts.Num())
	{
		return;
	}

	const float SafeGrow = FMath::Max(GrowDuration, 0.01f);
	const float GlowTime = FMath::Max(Settings->GlowDuration, 0.01f);
	bool bDirty = false;

	for (int32 Index = 0; Index < Sprouts.Num(); ++Index)
	{
		FIvySprout& Sprout = Sprouts[Index];

		float Scale = 0.f;
		float Glow = 0.f;
		if (Sprout.Birth <= GrowthCutoffTime && EffectTime >= Sprout.Birth)
		{
			const float Age = EffectTime - Sprout.Birth;
			Scale = IvyVFX::EaseOutBack(FMath::Clamp(Age / SafeGrow, 0.f, 1.f), Overshoot) * WitherFactor(Sprout.Birth);
			Glow = 1.f - FMath::Clamp(Age / GlowTime, 0.f, 1.f);
		}
		Scale = FMath::Max(Scale, IvyVFX::MinSproutScale);

		if (!FMath::IsNearlyEqual(Scale, Sprout.LastScale, 1.e-4f))
		{
			FTransform NewTransform = Sprout.WorldTransform;
			NewTransform.SetScale3D(Sprout.WorldTransform.GetScale3D() * Scale);
			Component->UpdateInstanceTransform(Index, NewTransform, true, false, true);
			Sprout.LastScale = Scale;
			bDirty = true;
		}
		if (!FMath::IsNearlyEqual(Glow, Sprout.LastGlow, 1.e-3f))
		{
			Component->SetCustomDataValue(Index, 0, Glow, false);
			Sprout.LastGlow = Glow;
			bDirty = true;
		}
	}

	if (bDirty)
	{
		Component->MarkRenderStateDirty();
	}
}

void AIvyVFXActor::UpdateTipFX()
{
	if (!TipFXComponent || LeadTimes.Num() == 0 || Phase != EIvyPhase::Growing)
	{
		return;
	}

	while (LeadIndex + 1 < LeadTimes.Num() && LeadTimes[LeadIndex + 1] <= EffectTime)
	{
		++LeadIndex;
	}

	if (LeadIndex + 1 >= LeadTimes.Num())
	{
		TipFXComponent->SetWorldLocation(LeadPoints.Last());
		TipFXComponent->Deactivate();
		return;
	}

	const float Span = LeadTimes[LeadIndex + 1] - LeadTimes[LeadIndex];
	const float Alpha = Span > KINDA_SMALL_NUMBER ? FMath::Clamp((EffectTime - LeadTimes[LeadIndex]) / Span, 0.f, 1.f) : 0.f;
	TipFXComponent->SetWorldLocation(FMath::Lerp(LeadPoints[LeadIndex], LeadPoints[LeadIndex + 1], Alpha));
}

void AIvyVFXActor::Wither()
{
	if (Phase == EIvyPhase::Idle || Phase == EIvyPhase::Withering || !Settings)
	{
		return;
	}

	if (Phase == EIvyPhase::Growing)
	{
		// Lo que aun no habia brotado ya no llega a salir.
		GrowthCutoffTime = EffectTime;
	}
	WitherBirthSpan = FMath::Min(MaxBirthTime, GrowthCutoffTime);
	WitherStartTime = EffectTime;
	Phase = EIvyPhase::Withering;

	if (TipFXComponent)
	{
		TipFXComponent->Deactivate();
	}

	if (Settings->WitherFX)
	{
		for (int32 NodeIndex = 0; NodeIndex < ActiveNodes.Num(); ++NodeIndex)
		{
			const bool bReached = NodeArrivalTimes.IsValidIndex(NodeIndex) && NodeArrivalTimes[NodeIndex] <= EffectTime;
			if (NodeHasIvy(NodeIndex) && bReached)
			{
				SpawnNodeFX(Settings->WitherFX, NodeIndex);
			}
		}
	}

	OnWitherStarted.Broadcast();
}

void AIvyVFXActor::FinishEffect()
{
	const TArray<FIvyNode> LoopNodes = ActiveNodes;

	Phase = EIvyPhase::Idle;
	ClearGeometry();

	OnFinished.Broadcast();
	if (!IsValid(this) || Phase != EIvyPhase::Idle)
	{
		return; // un listener nos ha destruido o ha relanzado el efecto
	}

	if (bAutoDestroy)
	{
		Destroy();
	}
	else if (bLoop && Settings && LoopNodes.Num() > 0)
	{
		StartEffect(LoopNodes);
	}
}

// El primer punto solo lleva hiedra si se enreda en el (o si es el unico punto).
bool AIvyVFXActor::NodeHasIvy(int32 NodeIndex) const
{
	return NodeIndex > 0 || ActiveNodes.Num() == 1 || (Settings->bWrapNodes && Settings->bWrapFirstNode);
}

void AIvyVFXActor::SpawnNodeFX(UNiagaraSystem* System, int32 NodeIndex) const
{
	if (!System || !ActiveNodes.IsValidIndex(NodeIndex) || !Settings)
	{
		return;
	}

	const FVector Location = ActiveNodes[NodeIndex].Location + IvyVFX::Up * Settings->ExtraFXHeight;
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, Location, FRotator::ZeroRotator, FVector(Settings->ExtraFXScale));
}

void AIvyVFXActor::DestroyTipFX()
{
	if (TipFXComponent)
	{
		TipFXComponent->DestroyComponent();
		TipFXComponent = nullptr;
	}
}
