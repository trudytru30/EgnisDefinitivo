#include "CoreAndSystems/AudioDataAsset.h"

// Se ejecuta al crear el objeto: cubre los AudioDataAsset NUEVOS, que nacen ya con todas las filas.
void UAudioDataAsset::PostInitProperties()
{
	Super::PostInitProperties();

	// El objeto por defecto de la clase es solo una plantilla, no un asset real, asi que no se rellena
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		FillMissingTriggers();
	}
}

// Se ejecuta tras cargar un asset ya guardado en disco: cubre los assets VIEJOS cuando se anade
// un momento nuevo al enum (esa fila aparece vacia al abrirlo). Hace falta ademas de PostInitProperties
// porque al cargar de disco los valores guardados sustituyen a lo que hubiera en la tabla.
void UAudioDataAsset::PostLoad()
{
	Super::PostLoad();

	FillMissingTriggers();
}

// Deja la tabla TriggerEvents con una fila por cada momento de EEgnisAudioTrigger.
// Las filas que ya existen no se tocan, asi que los eventos ya elegidos se conservan.
void UAudioDataAsset::FillMissingTriggers()
{
	// Informacion del enum en tiempo de ejecucion (cuantos valores tiene y cuales son)
	const UEnum* TriggerEnum = StaticEnum<EEgnisAudioTrigger>();
	if (!TriggerEnum)
	{
		return;
	}

	// NumEnums() - 1: el ultimo valor es el _MAX que genera Unreal solo, no es un momento real
	for (int32 i = 0; i < TriggerEnum->NumEnums() - 1; ++i)
	{
		// GetValueByIndex devuelve el numero del valor; se convierte de nuevo al enum
		const EEgnisAudioTrigger Trigger = static_cast<EEgnisAudioTrigger>(TriggerEnum->GetValueByIndex(i));

		// Solo se anade la fila si falta, con el evento FMOD vacio para que lo elija diseno
		if (!TriggerEvents.Contains(Trigger))
		{
			TriggerEvents.Add(Trigger, nullptr);
		}
	}
}