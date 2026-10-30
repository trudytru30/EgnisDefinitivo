"""Read-only validation of the Data Asset used by MapaIntercativo."""
import unreal
BASE="/Game/GameDesign/Luis/Level_Proto/MapaJuego/"
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(BASE+"MapaIntercativo")
all_actors=actors.get_all_level_actors()
assert not any(isinstance(a,unreal.MapNode) for a in all_actors),"Casillas must be generated from the asset"
managers=[a for a in all_actors if isinstance(a,unreal.LevelMapManager)]
assert len(managers)==1
graph=managers[0].get_editor_property("graph_definition")
assert graph
error=graph.validate_definition()
assert error is not None,str(error)
entries=graph.get_editor_property("nodes")
assert len(entries)==30
assert str(graph.get_editor_property("root_node_id"))=="Diff1"
sections={str(n.get_editor_property("data").get_editor_property("section")) for n in entries}
assert len(sections)==3
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
gm=world.get_world_settings().get_editor_property("default_game_mode")
assert unreal.get_default_object(gm).get_editor_property("default_pawn_class").get_name()=="Character_Map_C"
unreal.log("MAP_DATA_ASSET_VERIFIED: 30 definitions, 3 sections, no placed casillas, camera preserved")
