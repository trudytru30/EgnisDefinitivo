#pragma once

#include "CoreMinimal.h"

class ULevelDataAsset;

// Importa niveles exportados con Egnis Blockout: Export -> "Download all (.json)", o "Ver JSON de todos" -> Copy text.
// Cada nivel se reconoce por su id de la herramienta, así reimportar actualiza el mismo asset en vez de duplicarlo.
namespace EgnisBlockout
{
	// Importa en el asset su nivel del JSON: el de su id; si aún no tiene, el único nivel o el que se llame igual
	bool ImportIntoAsset(ULevelDataAsset* Asset, const FString& Json, const FString& SourceFile);

	// Vuelve a leer el archivo de la última importación del asset
	bool Reimport(ULevelDataAsset* Asset);

	// Crea o actualiza un asset por cada nivel del JSON (en la carpeta de Project Settings > Egnis > Egnis Blockout) y los guarda
	void ImportAll(const FString& Json, const FString& SourceFile);

	// Para los botones y el menú
	bool ReadClipboard(FString& OutText);
	bool PickJsonFile(FString& OutStoredPath, FString& OutText);
	void ImportAllFromClipboard();
	void ImportAllFromFile();
}
