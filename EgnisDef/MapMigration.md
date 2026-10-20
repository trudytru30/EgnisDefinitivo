# Mapa de niveles integrado

El mapa existente MapaIntercativo usa BP_MapNode_Base -> AMapNode y BP_MapManager -> ALevelMapManager. Las seis variantes de casilla conservan sus componentes visuales. La lógica antigua de selección se ha sustituido por C++.

Cada mapa es un grafo dirigido sin ciclos con tres secciones predefinidas (First, Second, Third). Cada casilla tiene en Data: NodeID, Section, NodeType, DifficultyLevel, Level y NextNodes. Los destinos son IDs; pueden existir bifurcaciones y destinos compartidos. No se genera el mapa aleatoriamente.

En Zone1 se conserva la ruta inicial y se completa el resto siguiendo la distribución visual. Diff1 es la raíz y B1 el boss final. La primera sección termina en Q1; la segunda abarca C3 hasta las rutas de T3/C5; la tercera reúne esas rutas hasta B1. Las dificultades originales de combate se conservan y las demás reciben valores iniciales editables.

Solo puede elegirse la raíz al empezar. Después de completar una casilla, solo se permiten sus destinos directos. No se puede repetir una casilla, saltar entre ramas, retroceder de sección ni omitir una sección. Todos los mapas deben contener casillas de las tres secciones.

## Probar el prototipo

1. Abrir /Game/Media/Level_Proto/MapaJuego/MapaIntercativo y pulsar Play.
2. Pulsar la esfera de Diff1 (verde). Las casillas grises están bloqueadas.
3. Se abre L_TestPartida. Pulsar Espacio para completar la casilla y volver.
4. Solo los destinos directos de la casilla completada se habilitan. Repetir hasta el boss.

Los niveles de destino siguen siendo L_TestPartida, igual que en los Blueprints originales. Esta integración no crea combates, tiendas o eventos definitivos. Para conectarlos, cambiar Data.Level y llamar a MapProgressionSubsystem.CompleteCurrentNode al resolver cada nivel, antes de regresar al mapa.

## Clases y estado

- AMapNode captura clics en sus componentes y actualiza el parámetro NodeColor del material existente. RefreshVisualState se puede ampliar en Blueprint.
- ALevelMapManager recoge y valida los nodos cargados, activa los clics del jugador y abre el nivel elegido.
- UMapProgressionSubsystem guarda el estado de la sesión entre cambios de mundo, independientemente de BP_GI_MapZ1. ResetProgress inicia otra run.
- AMapEncounterExit implementa Espacio -> completar -> volver en L_TestPartida.
- BP_MapMenuGameMode usa Character_Map para conservar la cámara del menú.

No hay SaveGame ni registro de casillas por streaming. La misma definición y MapID conservan el progreso; cambiar la definición lo reinicia. Los niveles de destino deben incluirse en el empaquetado.

Las copias originales previas a la migración están en Saved/MapMigrationBackup (carpeta local no versionada).

Pruebas: Egnis.Map.GraphProgression y Egnis.Map.BlueprintIntegration. La segunda ejecuta PIE, activa el delegado de clic del mesh Sphere, abre L_TestPartida y regresa al mapa comprobando el estado.
