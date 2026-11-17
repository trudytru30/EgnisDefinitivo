#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IvyVFXActor.generated.h"

class UIvyVFXSettings;
class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraComponent;
class UStaticMesh;
struct FRandomStream;

/** Un punto por el que pasa la hiedra (normalmente, una pieza). */
USTRUCT(BlueprintType)
struct FIvyNode
{
	GENERATED_BODY()

	// Posicion en el mundo de la BASE de la pieza (a ras de suelo).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ivy")
	FVector Location = FVector::ZeroVector;

	// Radio de la pieza para el enredo. Negativo = usar el WrapRadius del DataAsset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ivy")
	float Radius = -1.f;

	// Altura hasta la que trepa la hiedra. Negativo = usar el WrapHeight del DataAsset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ivy")
	float Height = -1.f;
};

UENUM(BlueprintType)
enum class EIvyPhase : uint8
{
	Idle UMETA(DisplayName = "Parado"),
	Growing UMETA(DisplayName = "Creciendo"),
	Holding UMETA(DisplayName = "Crecida"),
	Withering UMETA(DisplayName = "Marchitandose"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIvyNodeReachedSignature, int32, NodeIndex, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIvyPhaseSignature);

/**
 * VFX de hiedra que crece de punto a punto y se enreda en las piezas.
 *
 * Uso desde gameplay (C++ o Blueprint):
 *   AIvyVFXActor::SpawnIvyThroughActors(this, Settings, { Lanzador, Objetivo1, Objetivo2 });
 *   AIvyVFXActor::SpawnIvyThroughPoints(this, Settings, { PosA, PosB, PosC });
 * El actor se destruye solo al terminar. OnNodeReached avisa cuando la hiedra llega a cada pieza
 * (el momento bueno para aplicar el dano / estado de la carta).
 *
 * Uso para disenar: arrastra el actor a un nivel, asignale un DataAsset de Settings, mueve los
 * Preview Points y pulsa "Preview In Editor" en el panel de detalles (con Realtime activado en el viewport).
 */
UCLASS(Blueprintable)
class EGNISDEF_API AIvyVFXActor : public AActor
{
	GENERATED_BODY()

public:
	AIvyVFXActor();

	// ===== Lanzar el efecto =====

	/** Crea el efecto pasando posiciones sueltas (base de cada pieza, a ras de suelo). Se destruye solo al terminar. */
	UFUNCTION(BlueprintCallable, Category = "Ivy VFX", meta = (WorldContext = "WorldContextObject"))
	static AIvyVFXActor* SpawnIvyThroughPoints(const UObject* WorldContextObject, UIvyVFXSettings* InSettings, const TArray<FVector>& Points);

	/** Crea el efecto pasando puntos con radio y altura propios. Se destruye solo al terminar. */
	UFUNCTION(BlueprintCallable, Category = "Ivy VFX", meta = (WorldContext = "WorldContextObject"))
	static AIvyVFXActor* SpawnIvyThroughNodes(const UObject* WorldContextObject, UIvyVFXSettings* InSettings, const TArray<FIvyNode>& InNodes);

	/** Crea el efecto pasando actores (piezas). Usa la base y el tamano de cada actor. Se destruye solo al terminar. */
	UFUNCTION(BlueprintCallable, Category = "Ivy VFX", meta = (WorldContext = "WorldContextObject"))
	static AIvyVFXActor* SpawnIvyThroughActors(const UObject* WorldContextObject, UIvyVFXSettings* InSettings, const TArray<AActor*>& Actors);

	/** Convierte un actor en un punto de hiedra (base del actor + radio y altura segun su colision). */
	UFUNCTION(BlueprintCallable, Category = "Ivy VFX")
	static FIvyNode MakeIvyNodeFromActor(const AActor* Actor, const UIvyVFXSettings* InSettings);

	/** (Re)lanza el efecto en este actor con estos puntos. */
	UFUNCTION(BlueprintCallable, Category = "Ivy VFX")
	void PlayThroughNodes(const TArray<FIvyNode>& InNodes);

	UFUNCTION(BlueprintCallable, Category = "Ivy VFX")
	void PlayThroughPoints(const TArray<FVector>& Points);

	UFUNCTION(BlueprintCallable, Category = "Ivy VFX")
	void PlayThroughActors(const TArray<AActor*>& Actors);

	/** Empieza a marchitar la hiedra ya (aunque siga creciendo). Necesario si el DataAsset tiene bStayUntilWither. */
	UFUNCTION(BlueprintCallable, Category = "Ivy VFX")
	void Wither();

	/** Corta el efecto en seco y borra toda la hiedra. No lanza OnFinished. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ivy VFX")
	void StopAndClear();

	/** Avanza el efecto a mano. Tick lo llama solo; sirve para previsualizar o para tests. */
	UFUNCTION(BlueprintCallable, Category = "Ivy VFX")
	void AdvanceEffect(float DeltaSeconds);

	/** Lanza el efecto en el editor usando Preview Actors (si hay) o Preview Points. El viewport debe estar en Realtime. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ivy VFX")
	void PreviewInEditor();

	UFUNCTION(BlueprintPure, Category = "Ivy VFX")
	EIvyPhase GetPhase() const { return Phase; }

	/** Numero de puntos por los que pasa la hiedra (el origen cuenta). */
	UFUNCTION(BlueprintPure, Category = "Ivy VFX")
	int32 GetNodeCount() const { return ActiveNodes.Num(); }

	/** Posicion en el mundo (base de la pieza) del punto NodeIndex. Vector cero si no existe. */
	UFUNCTION(BlueprintPure, Category = "Ivy VFX")
	FVector GetNodeLocation(int32 NodeIndex) const { return ActiveNodes.IsValidIndex(NodeIndex) ? ActiveNodes[NodeIndex].Location : FVector::ZeroVector; }

	/** Segundos (ya con PlayRate aplicado) desde que empezo el efecto hasta que la hiedra llega al punto indicado. -1 si no existe. */
	UFUNCTION(BlueprintPure, Category = "Ivy VFX")
	float GetNodeArrivalTime(int32 NodeIndex) const;

	// ===== Configuracion =====

	// DataAsset con todos los parametros visuales y de ritmo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ivy VFX")
	TObjectPtr<UIvyVFXSettings> Settings;

	// Destruir el actor cuando el efecto termina. Los Spawn... lo activan solos; dejalo apagado en actores colocados a mano.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ivy VFX")
	bool bAutoDestroy = false;

	// Lanzar el efecto al empezar la partida usando Preview Actors / Preview Points.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ivy VFX|Preview")
	bool bPlayOnBeginPlay = false;

	// Repetir el efecto en bucle al terminar (para ajustar parametros mirando el resultado).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ivy VFX|Preview")
	bool bLoop = false;

	// Puntos de prueba, relativos al actor. Se pueden mover con el gizmo en el viewport.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ivy VFX|Preview", meta = (MakeEditWidget = true))
	TArray<FVector> PreviewPoints;

	// Actores de prueba del nivel. Si hay alguno, se usan en vez de los Preview Points.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Ivy VFX|Preview")
	TArray<TObjectPtr<AActor>> PreviewActors;

	// ===== Eventos =====

	// La hiedra acaba de llegar al punto NodeIndex (el 0 es el origen y salta nada mas empezar).
	UPROPERTY(BlueprintAssignable, Category = "Ivy VFX")
	FIvyNodeReachedSignature OnNodeReached;

	// La hiedra ha terminado de crecer del todo.
	UPROPERTY(BlueprintAssignable, Category = "Ivy VFX")
	FIvyPhaseSignature OnGrowthFinished;

	// La hiedra empieza a marchitarse.
	UPROPERTY(BlueprintAssignable, Category = "Ivy VFX")
	FIvyPhaseSignature OnWitherStarted;

	// El efecto ha terminado y ya no queda nada en pantalla.
	UPROPERTY(BlueprintAssignable, Category = "Ivy VFX")
	FIvyPhaseSignature OnFinished;

	// ===== AActor =====
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Ivy VFX|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Ivy VFX|Components")
	TObjectPtr<UProceduralMeshComponent> StemMesh;

	UPROPERTY(VisibleAnywhere, Category = "Ivy VFX|Components")
	TObjectPtr<UInstancedStaticMeshComponent> LeafInstances;

	UPROPERTY(VisibleAnywhere, Category = "Ivy VFX|Components")
	TObjectPtr<UInstancedStaticMeshComponent> FlowerInstances;

private:
	// ---- Datos generados (no reflejados) ----

	/** Superficie sobre la que crece un tallo: el suelo o el cilindro que envuelve a una pieza. */
	struct FIvySurface
	{
		bool bCylinder = false;
		FVector Center = FVector::ZeroVector;
		float Radius = 0.f;
		float Height = 0.f;
		float TopRadiusScale = 1.f;
	};

	/** Recorrido de un tallo antes de convertirlo en geometria. Todo en espacio de mundo. */
	struct FIvyPath
	{
		TArray<FVector> Points;
		TArray<FVector> Normals;   // normal de la superficie en cada punto
		TArray<float> Times;       // segundo en el que la punta pasa por cada punto
		TArray<int32> Surfaces;    // indice en SurfaceList

		void Add(const FVector& P, const FVector& N, float T, int32 S)
		{
			Points.Add(P); Normals.Add(N); Times.Add(T); Surfaces.Add(S);
		}
	};

	/** Un anillo del tubo del tallo (espacio local del actor). */
	struct FIvyRing
	{
		FVector Center;
		FVector AxisN;
		FVector AxisB;
		float Radius;
		float Birth;
		FLinearColor Color;
	};

	/** Una hoja o flor instanciada. */
	struct FIvySprout
	{
		FTransform WorldTransform; // con la escala final
		float Birth = 0.f;
		float LastScale = -1.f;
		float LastGlow = -1.f;
	};

	struct FIvyNodeEvent
	{
		float Time = 0.f;
		int32 NodeIndex = 0;
	};

	TArray<FIvyNode> ActiveNodes;
	TArray<FIvySurface> SurfaceList;
	TArray<FIvyRing> Rings;
	TArray<FIvySprout> Leaves;
	TArray<FIvySprout> Flowers;
	TArray<FIvyNodeEvent> NodeEvents;
	TArray<float> NodeArrivalTimes;
	TArray<float> LeadTimes;
	TArray<FVector> LeadPoints;

	// Buffers de la malla de tallos
	TArray<FVector> StemVertices;
	TArray<FLinearColor> StemColors;
	TArray<int32> StemTriangles;
	TArray<FVector> StemNormals;
	TArray<FVector2D> StemUVs;
	TArray<float> RingCos;
	TArray<float> RingSin;
	int32 RingSides = 6;
	int32 NumStrands = 0;

	EIvyPhase Phase = EIvyPhase::Idle;
	float EffectTime = 0.f;
	float GrowEndTime = 0.f;      // cuando acaba toda animacion de crecimiento
	float MaxBirthTime = 0.f;     // nacimiento mas tardio (para normalizar el marchitado)
	float GrowthCutoffTime = 0.f; // lo que nazca despues de este instante ya no aparece
	float HoldEndTime = 0.f;
	float WitherStartTime = 0.f;
	float WitherBirthSpan = 0.f;  // rango de nacimientos visible al empezar a marchitar
	int32 NextNodeEvent = 0;
	int32 LeadIndex = 0;
	bool bGeometryBuilt = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StemMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LeafMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlowerMID;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> TipFXComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> TraceIgnoredActors;

	// ---- Generacion ----
	void StartEffect(const TArray<FIvyNode>& InNodes);
	void ClearGeometry();
	void BuildIvy();
	void BuildLink(int32 FromIndex, int32 ToIndex, float StartTime, FRandomStream& Rng, float& OutArrivalTime);
	void BuildWrapOnly(int32 NodeIndex, float StartTime, FRandomStream& Rng);
	void AppendHelix(FIvyPath& Path, int32 SurfaceIndex, float StartAngle, float Direction, float StartTime) const;
	void GrowBranch(FIvyPath& OutPath, const FVector& Start, const FVector& Tangent, const FVector& Normal, int32 SurfaceIndex,
		float StartTime, float Speed, int32 Depth, FRandomStream& Rng) const;
	void AddStrand(const FIvyPath& Path, float BaseRadius, float Speed, int32 Depth, FRandomStream& Rng);
	void ProjectToSurface(int32 SurfaceIndex, FVector& InOutPoint, FVector& OutNormal) const;
	int32 AddNodeSurface(int32 NodeIndex);
	float ResolveNodeRadius(const FIvyNode& Node) const;
	float ResolveNodeHeight(const FIvyNode& Node) const;
	void CommitGeometry();
	void SetupSproutComponent(UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, UMaterialInterface* Material,
		const FLinearColor& TintA, const FLinearColor& TintB, TObjectPtr<UMaterialInstanceDynamic>& OutMID,
		TArray<FIvySprout>& Sprouts, FRandomStream& Rng);

	// ---- Animacion ----
	void UpdateVisuals();
	void UpdateSprouts(UInstancedStaticMeshComponent* Component, TArray<FIvySprout>& Sprouts, float GrowDuration, float Overshoot);
	void UpdateTipFX();
	float WitherFactor(float Birth) const;
	void FinishEffect();
	void SpawnNodeFX(class UNiagaraSystem* System, int32 NodeIndex) const;
	void DestroyTipFX();
	bool IsEditorPreviewWorld() const;
	bool NodeHasIvy(int32 NodeIndex) const;
};
