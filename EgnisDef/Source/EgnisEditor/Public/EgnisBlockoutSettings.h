#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CoreAndSystems/Board.h"
#include "EgnisBlockoutSettings.generated.h"

// Cómo se traduce cada clave del JSON de Egnis Blockout al motor (Project Settings > Egnis > Egnis Blockout)
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Egnis Blockout"))
class EGNISEDITOR_API UEgnisBlockoutSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEgnisBlockoutSettings();

	// Claves de casilla de la herramienta (WALL, WATER, BREAKABLE...) y su tipo en el tablero.
	// Las claves que no estén aquí ni en Enemy Classes se importan como Normal y avisan
	UPROPERTY(Config, EditAnywhere, Category = "Casillas")
	TMap<FString, ETileType> TileTypes;

	// Claves de enemigo de la herramienta (APERO, UROU...) y su Blueprint
	UPROPERTY(Config, EditAnywhere, Category = "Enemigos")
	TMap<FString, TSoftClassPtr<AActor>> EnemyClasses;

	// Claves que se importan como casilla Normal sin avisar (P1-P4 solo sirven para el balance de la herramienta)
	UPROPERTY(Config, EditAnywhere, Category = "Casillas")
	TArray<FString> IgnoredKeys;

	// Carpeta donde "Importar todos" crea los niveles. {Zone} es el número de zona del JSON ("ZONE 1" -> 1)
	UPROPERTY(Config, EditAnywhere, Category = "Importar")
	FString LevelsFolder;

	UPROPERTY(Config, EditAnywhere, Category = "Importar")
	FString AssetPrefix;
};
