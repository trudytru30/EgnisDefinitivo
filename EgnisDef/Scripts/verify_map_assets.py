"""Read-only asset validation. Run via UnrealEditor-Cmd -run=pythonscript."""
import unreal,json
BASE="/Game/Media/Level_Proto/MapaJuego/"
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level(BASE+"MapaIntercativo")
nodes=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.MapNode)]
managers=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.LevelMapManager)]
assert len(nodes)==30 and len(managers)==1
definitions=[a.get_editor_property("data") for a in nodes]
progress=unreal.new_object(unreal.MapProgressionSubsystem,outer=unreal.new_object(unreal.GameInstance))
error=progress.initialize_map("Zone1",definitions,"Diff1")
assert error is not None and progress.is_node_accessible("Diff1"),str(error)
rows=[]
sections=set()
for a,data in zip(nodes,definitions):
    sections.add(str(data.get_editor_property("section")))
    components=a.get_components_by_class(unreal.StaticMeshComponent)
    assert {"Base","Sphere"}.issubset({c.get_name() for c in components}),a.get_actor_label()
    level=data.get_editor_property("level")
    assert level is not None
    rows.append({"id":str(data.get_editor_property("node_id")),"label":a.get_actor_label(),
        "type":str(data.get_editor_property("node_type")),"section":str(data.get_editor_property("section")),
        "difficulty":data.get_editor_property("difficulty_level"),
        "next":[str(n) for n in data.get_editor_property("next_nodes")],
        "level":level.get_path_name()})
assert len(sections)==3
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
gm=world.get_world_settings().get_editor_property("default_game_mode")
assert unreal.get_default_object(gm).get_editor_property("default_pawn_class").get_name()=="Character_Map_C"
levels.load_level(BASE+"L_TestPartida")
exits=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.MapEncounterExit)]
assert len(exits)==1
assert exits[0].get_editor_property("return_map") is not None
unreal.log("MAP_ASSETS_VERIFIED: 30 nodes, visual meshes, camera, 3 sections and prototype exit")
with open(unreal.Paths.project_saved_dir()+"MapGraphZone1.json","w",encoding="utf-8") as f:
    json.dump({"map_id":"Zone1","root":"Diff1","nodes":rows},f,indent=2,ensure_ascii=False)
