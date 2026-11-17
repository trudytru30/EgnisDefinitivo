#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "IvyVFXSettings.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UNiagaraSystem;

/** Como se reparte la hiedra entre los puntos que le pasan. */
UENUM(BlueprintType)
enum class EIvyPropagationMode : uint8
{
	// 0 -> 1 -> 2 -> 3 ... La hiedra salta de pieza en pieza, en orden.
	Chain UMETA(DisplayName = "Cadena (de pieza en pieza)"),
	// 0 -> 1, 0 -> 2, 0 -> 3 ... Todas las ramas salen a la vez del primer punto.
	FromFirst UMETA(DisplayName = "Abanico (todo sale del primer punto)"),
};

/** Como se calcula lo que tarda la hiedra en ir de un punto al siguiente. */
UENUM(BlueprintType)
enum class EIvyTimingMode : uint8
{
	// Cada tramo tarda siempre lo mismo, este lejos o cerca. Lo mas comodo para sincronizar con gameplay.
	FixedDurationPerLink UMETA(DisplayName = "Duracion fija por tramo"),
	// La hiedra avanza a velocidad constante: los tramos largos tardan mas.
	ConstantSpeed UMETA(DisplayName = "Velocidad constante"),
};

/** Como desaparece la hiedra al final. */
UENUM(BlueprintType)
enum class EIvyWitherMode : uint8
{
	// Se recoge desde la ultima pieza hacia el origen (como si rebobinase).
	RetractToOrigin UMETA(DisplayName = "Se recoge hacia el origen"),
	// Desaparece en el mismo orden en el que crecio (primero el origen, al final la ultima pieza).
	FollowGrowth UMETA(DisplayName = "Desaparece en el orden en que crecio"),
	// Todo se encoge a la vez.
	AllAtOnce UMETA(DisplayName = "Todo a la vez"),
};

/**
 * Todos los parametros del VFX de hiedra (AIvyVFXActor).
 * Crea uno desde el Content Browser: clic derecho > Miscellaneous > Data Asset > IvyVFXSettings.
 * Puedes tener tantos como quieras (uno por carta, por culto, etc.) y pasarselos al efecto al lanzarlo.
 * Todas las distancias estan en centimetros (una casilla = 100) y los tiempos en segundos.
 */
UCLASS(BlueprintType)
class EGNISDEF_API UIvyVFXSettings : public UDataAsset
{
	GENERATED_BODY()

public:

#pragma region Ritmo

	// Multiplicador global de velocidad de TODO el efecto (2 = el doble de rapido, 0.5 = camara lenta).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo", meta = (ClampMin = "0.05", UIMax = "4"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo")
	EIvyTimingMode TimingMode = EIvyTimingMode::FixedDurationPerLink;

	// Segundos que tarda la hiedra en ir de un punto al siguiente.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo",
		meta = (ClampMin = "0.05", EditCondition = "TimingMode == EIvyTimingMode::FixedDurationPerLink", EditConditionHides))
	float LinkDuration = 0.55f;

	// Velocidad de avance en cm por segundo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo",
		meta = (ClampMin = "1", EditCondition = "TimingMode == EIvyTimingMode::ConstantSpeed", EditConditionHides))
	float GrowthSpeed = 350.f;

	// Pausa al llegar a cada pieza antes de saltar a la siguiente (solo en modo Cadena).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo", meta = (ClampMin = "0"))
	float NodePause = 0.12f;

	// Desfase aleatorio maximo entre los tallos de un mismo tramo, para que no avancen todos como un bloque.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo", meta = (ClampMin = "0"))
	float StemStagger = 0.1f;

	// Segundos que la hiedra se queda quieta, ya crecida, antes de marchitarse.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo", meta = (ClampMin = "0", EditCondition = "!bStayUntilWither"))
	float HoldDuration = 0.8f;

	// Si se activa, la hiedra NO se marchita sola: se queda hasta que alguien llame a Wither() en el actor.
	// Util para estados que duran varios turnos (por ejemplo, enraizado).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo")
	bool bStayUntilWither = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo")
	EIvyWitherMode WitherMode = EIvyWitherMode::RetractToOrigin;

	// Segundos que tarda la "ola" de marchitado en recorrer toda la hiedra.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo", meta = (ClampMin = "0"))
	float WitherDuration = 0.7f;

	// Segundos que tarda cada trozo de tallo / hoja en encogerse cuando le llega el marchitado.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritmo", meta = (ClampMin = "0.01"))
	float WitherFade = 0.3f;

#pragma endregion

#pragma region Recorrido

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido")
	EIvyPropagationMode PropagationMode = EIvyPropagationMode::Chain;

	// Cuantos tallos principales viajan de un punto al siguiente.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido", meta = (ClampMin = "1", ClampMax = "12"))
	int32 StemsPerLink = 3;

	// Anchura total del "camino" que ocupan los tallos entre dos puntos (cuanto se abren hacia los lados).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido", meta = (ClampMin = "0"))
	float PathWidth = 45.f;

	// Ondulaciones por metro del recorrido. Mas alto = tallos mas nerviosos.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido", meta = (ClampMin = "0"))
	float NoiseFrequency = 1.6f;

	// Cuanto suben y bajan los tallos de forma irregular a lo largo del recorrido.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido", meta = (ClampMin = "0"))
	float VerticalNoise = 5.f;

	// Altura del arco que hacen los tallos entre dos puntos. 0 = pegados al suelo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido")
	float ArcHeight = 0.f;

	// Separacion sobre el suelo para que los tallos no se metan dentro del tablero.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido")
	float GroundOffset = 2.5f;

	// Si se activa, cada trozo del recorrido lanza un rayo hacia abajo y se pega a lo que encuentre
	// (sirve para suelos con desniveles). En un tablero plano no hace falta.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido")
	bool bProjectToGround = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido", meta = (EditCondition = "bProjectToGround"))
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// Distancia hacia arriba y hacia abajo que cubre el rayo de busqueda de suelo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recorrido", meta = (ClampMin = "1", EditCondition = "bProjectToGround"))
	float GroundTraceDistance = 150.f;

#pragma endregion

#pragma region Tallos

	// Material de los tallos. Recibe el color por Vertex Color (RGB) y el brillo de crecimiento en el alfa.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos")
	TObjectPtr<UMaterialInterface> StemMaterial;

	// Cada tallo coge un color al azar entre estos dos.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos")
	FLinearColor StemColorA = FLinearColor(0.08f, 0.14f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos")
	FLinearColor StemColorB = FLinearColor(0.16f, 0.11f, 0.04f);

	// Radio (grosor) de los tallos principales.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos", meta = (ClampMin = "0.05"))
	float StemRadius = 1.8f;

	// Grosor de la punta respecto a la base (0 = acaba en punta, 1 = igual de gordo en todo el tallo).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos", meta = (ClampMin = "0", ClampMax = "1"))
	float TipRadiusScale = 0.3f;

	// Segundos que tarda un trozo de tallo en engordar desde que le pasa la punta. Mas alto = punta mas afilada.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos", meta = (ClampMin = "0.01"))
	float TipTaperTime = 0.22f;

	// Longitud de cada segmento de tallo. Mas bajo = curvas mas suaves pero mas geometria.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos", meta = (ClampMin = "2", ClampMax = "50"))
	float SegmentLength = 6.f;

	// Lados del tubo del tallo (3 = triangular, 6 = suficiente para vista de tablero).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos", meta = (ClampMin = "3", ClampMax = "12"))
	int32 RadialSegments = 6;

	// Cada cuantos cm se repite la textura a lo largo del tallo (solo importa si el material usa textura).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tallos", meta = (ClampMin = "1"))
	float UVLengthScale = 20.f;

#pragma endregion

#pragma region Ramas

	// Ramas secundarias que brotan por cada metro de tallo. 0 = sin ramas.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramas", meta = (ClampMin = "0", UIMax = "10"))
	float BranchesPerMeter = 3.f;

	// Generaciones de ramas: 1 = solo ramas del tallo principal, 2 = las ramas tambien echan ramas...
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramas", meta = (ClampMin = "0", ClampMax = "4"))
	int32 MaxBranchDepth = 2;

	// Longitud de cada rama (se elige al azar entre minimo y maximo).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramas")
	FFloatInterval BranchLength = FFloatInterval(20.f, 55.f);

	// Cuanto se acortan las ramas en cada generacion.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramas", meta = (ClampMin = "0.1", ClampMax = "1"))
	float BranchLengthDepthScale = 0.6f;

	// Angulo (grados) con el que la rama sale del tallo padre.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramas")
	FFloatInterval BranchAngle = FFloatInterval(30.f, 70.f);

	// Cuanto se riza la rama mientras crece (grados por metro). 0 = ramas rectas.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramas", meta = (ClampMin = "0"))
	float BranchCurl = 260.f;

	// Grosor de la rama respecto al punto del tallo del que nace.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramas", meta = (ClampMin = "0.1", ClampMax = "1"))
	float BranchRadiusScale = 0.65f;

	// Velocidad de crecimiento de las ramas respecto al tallo principal.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramas", meta = (ClampMin = "0.1", UIMax = "2"))
	float BranchSpeedScale = 0.7f;

#pragma endregion

#pragma region Enredo

	// Al llegar a cada pieza, los tallos trepan en espiral a su alrededor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas")
	bool bWrapNodes = true;

	// Enredar tambien el primer punto (la pieza que lanza el efecto).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (EditCondition = "bWrapNodes"))
	bool bWrapFirstNode = false;

	// Radio de la espiral cuando el punto no trae radio propio (por ejemplo al pasar posiciones sueltas).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "1"))
	float WrapRadius = 28.f;

	// Altura hasta la que trepa cuando el punto no trae altura propia.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "0"))
	float WrapHeight = 85.f;

	// Separacion entre la hiedra y la superficie de la pieza, para que no quede metida dentro de la malla.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "0"))
	float WrapSurfaceOffset = 4.f;

	// Vueltas que da cada tallo alrededor de la pieza.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "0", UIMax = "5"))
	float WrapTurns = 1.4f;

	// Radio de la espiral arriba del todo respecto al de abajo (menos de 1 = se cierra hacia arriba).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "0.05", UIMax = "2"))
	float WrapTopRadiusScale = 0.75f;

	// Segundos que tarda en trepar la pieza.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "0.01"))
	float WrapDuration = 0.45f;

	// Angulo (grados) en el que se reparten los tallos al llegar a la pieza. 360 = la rodean por completo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "0", ClampMax = "360"))
	float WrapArrivalSpread = 150.f;

	// Al lanzar el efecto sobre actores (no sobre posiciones), usar el tamano del actor como radio y altura del enredo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas")
	bool bUseActorBounds = true;

	// Multiplicador del radio del actor (1 = justo en su colision).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "0.1", EditCondition = "bUseActorBounds"))
	float ActorRadiusScale = 1.f;

	// Fraccion de la altura del actor hasta la que trepa la hiedra (0.6 = hasta el 60%).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enredo en las piezas", meta = (ClampMin = "0", UIMax = "1.2", EditCondition = "bUseActorBounds"))
	float ActorHeightScale = 0.6f;

#pragma endregion

#pragma region Hojas

	// Malla de la hoja. Convencion: el peciolo (donde se engancha al tallo) en el origen, la hoja apuntando a +X
	// y la cara de arriba mirando a +Z. Si se deja vacio no hay hojas.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas")
	TObjectPtr<UStaticMesh> LeafMesh;

	// Material de las hojas. Si se deja vacio se usa el que tenga la malla.
	// Parametros que el efecto intenta rellenar: TintA, TintB, GlowColor.
	// PerInstanceCustomData 0 = brillo de crecimiento (1 -> 0), 1 = numero al azar por hoja (0..1).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas")
	TObjectPtr<UMaterialInterface> LeafMaterial;

	// Cada hoja coge un color al azar entre estos dos.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas")
	FLinearColor LeafColorA = FLinearColor(0.07f, 0.30f, 0.04f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas")
	FLinearColor LeafColorB = FLinearColor(0.22f, 0.48f, 0.08f);

	// Hojas por cada metro de tallo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas", meta = (ClampMin = "0", UIMax = "40"))
	float LeavesPerMeter = 16.f;

	// Tamano de cada hoja (al azar entre minimo y maximo).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas")
	FFloatInterval LeafScale = FFloatInterval(0.7f, 1.4f);

	// Segundos que tarda en brotar la hoja desde que el tallo pasa por ahi (al azar entre minimo y maximo).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas")
	FFloatInterval LeafDelay = FFloatInterval(0.05f, 0.3f);

	// Segundos que tarda la hoja en abrirse del todo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas", meta = (ClampMin = "0.01"))
	float LeafGrowDuration = 0.35f;

	// Rebote al abrirse (0 = sin rebote, 1 = rebote marcado tipo "pop").
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas", meta = (ClampMin = "0", UIMax = "2"))
	float LeafOvershoot = 0.8f;

	// Cuanto se levanta la hoja del suelo, en grados (al azar entre minimo y maximo).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hojas")
	FFloatInterval LeafTilt = FFloatInterval(10.f, 45.f);

#pragma endregion

#pragma region Flores

	// Malla de la flor. Convencion: base en el origen y la flor abierta mirando a +Z. Vacio = sin flores.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores")
	TObjectPtr<UStaticMesh> FlowerMesh;

	// Material de las flores. Vacio = el de la malla. Mismos parametros que el de las hojas.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores")
	TObjectPtr<UMaterialInterface> FlowerMaterial;

	// Cada flor coge un color al azar entre estos dos.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores")
	FLinearColor FlowerColorA = FLinearColor(1.f, 0.85f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores")
	FLinearColor FlowerColorB = FLinearColor(1.f, 0.55f, 0.7f);

	// Flores por cada metro de tallo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores", meta = (ClampMin = "0", UIMax = "20"))
	float FlowersPerMeter = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores")
	FFloatInterval FlowerScale = FFloatInterval(1.f, 1.8f);

	// Segundos que tarda en abrirse la flor desde que el tallo pasa por ahi. Lo normal es que salgan despues de las hojas.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores")
	FFloatInterval FlowerDelay = FFloatInterval(0.2f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores", meta = (ClampMin = "0.01"))
	float FlowerGrowDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flores", meta = (ClampMin = "0", UIMax = "2"))
	float FlowerOvershoot = 1.2f;

#pragma endregion

#pragma region Brillo

	// Color del destello que lleva la hiedra mientras crece (tallos, hojas y flores). Admite valores HDR.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brillo", meta = (HDR))
	FLinearColor GlowColor = FLinearColor(0.25f, 1.f, 0.15f);

	// Intensidad del destello. 0 = sin brillo.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brillo", meta = (ClampMin = "0", UIMax = "50"))
	float GlowIntensity = 5.f;

	// Segundos que tarda en apagarse el destello de cada trozo despues de aparecer.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brillo", meta = (ClampMin = "0.01"))
	float GlowDuration = 0.5f;

#pragma endregion

#pragma region Extras

	// Sistema Niagara que viaja pegado a la punta del tallo guia mientras la hiedra crece (chispas, polen...).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Efectos extra (Niagara)")
	TObjectPtr<UNiagaraSystem> TipFX;

	// Sistema Niagara que se lanza en cada pieza justo cuando la hiedra llega a ella.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Efectos extra (Niagara)")
	TObjectPtr<UNiagaraSystem> NodeReachedFX;

	// Sistema Niagara que se lanza en cada pieza cuando la hiedra empieza a marchitarse (hojas cayendo...).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Efectos extra (Niagara)")
	TObjectPtr<UNiagaraSystem> WitherFX;

	// Escala de los sistemas Niagara anteriores.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Efectos extra (Niagara)", meta = (ClampMin = "0.01"))
	float ExtraFXScale = 1.f;

	// Altura sobre la base de la pieza a la que se lanzan NodeReachedFX y WitherFX.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Efectos extra (Niagara)")
	float ExtraFXHeight = 0.f;

#pragma endregion

#pragma region Avanzado

	// Semilla del azar. 0 = cada vez sale una hiedra distinta. Cualquier otro numero = siempre la misma forma.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avanzado")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avanzado")
	bool bCastShadows = true;

	// Limites de seguridad para que una mala combinacion de parametros no genere una barbaridad de geometria.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avanzado", meta = (ClampMin = "1", ClampMax = "2000"))
	int32 MaxStrands = 400;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avanzado", meta = (ClampMin = "0", ClampMax = "20000"))
	int32 MaxLeaves = 3000;

#pragma endregion
};
