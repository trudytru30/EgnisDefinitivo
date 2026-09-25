#pragma once

#include "NativeGameplayTags.h"

// Tag usado para pasar la cantidad de daño a un GameplayEffect en tiempo de ejecución
// (Set By Caller), sin tener que crear un GE distinto por cada valor de daño posible.
// Nativo en vez de definido en Project Settings: se registra solo al arrancar, sin
// necesidad de tocar ninguna configuración del editor.
EGNISDEF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Damage);