#pragma once

// Header compartido solo para la macro de accessors de atributos GAS. Al vivir aparte,
// los Attribute pueden incluirla sin depender del orden en que el compilador procese 
// los archivos.

// GAS espera que cada atributo tenga 4 funciones: un getter que devuelve la propiedad entera
// (FGameplayAttribute), un getter del valor float, un setter, y un "initter" para poner el valor
// inicial sin pasar por la maquinaria de GameplayEffects. Esta macro genera esas 4 funciones.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)