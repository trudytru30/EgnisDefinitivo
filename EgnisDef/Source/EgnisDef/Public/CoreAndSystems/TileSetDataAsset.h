#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Board.h"
#include "TileSetDataAsset.generated.h"

class UStaticMesh;
class UMaterialInterface;

// Cómo es un tipo de casilla: el cubo de suelo y, si lo tiene, lo que se coloca encima
USTRUCT(BlueprintType)
struct FTileTypeDefinition
{
	GENERATED_BODY()

	// Cubo de suelo de este tipo: TileSize de lado y pivote en el centro de la cara de arriba.
	// Si se deja vacío se usa el de Normal (con su altura)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UStaticMesh> FloorMesh;

	// Opcional: sustituye al material del cubo (p. ej. el mismo cubo en azul para el agua)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UMaterialInterface> FloorMaterial;

	// Sube o baja el cubo para que su cara de arriba quede a la altura de la casilla.
	// El Cube del motor tiene el pivote en el centro: necesita -50
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	float HeightOffset = 0.f;

	// Opcional: lo que se coloca encima de cada casilla de este tipo. Si es un obstáculo (hijo de AObstacleBase)
	// ocupa la casilla y no se puede pasar; si es otra cosa (p. ej. partículas de niebla), es solo decorado
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TSubclassOf<AActor> ActorOnTop;
};

// Catálogo de casillas: qué cubo y qué actor lleva cada tipo. El Level Data Asset dice qué tipo hay en cada
// casilla y el tablero (ABoard::TileSet) busca aquí cómo es ese tipo al cargar el nivel
UCLASS()
class EGNISDEF_API UTileSetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// Viene con todos los tipos ya puestos (menos Void, que es un hueco y nunca lleva nada)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile Set")
	TMap<ETileType, FTileTypeDefinition> TileTypes;

	const FTileTypeDefinition* FindDefinition(ETileType Type) const { return TileTypes.Find(Type); }

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

private:
	// Añade los tipos que falten (assets nuevos, o si se añade un tipo nuevo a ETileType)
	void AddMissingTileTypes();
};
