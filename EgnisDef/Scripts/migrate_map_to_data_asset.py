"""Convert the legacy prototype to a Data Asset, preserving the original Blueprints."""
import unreal,os,shutil,math
BASE="/Game/GameDesign/Luis/Level_Proto/MapaJuego/"
OUT=BASE+"Blueprints/DataDriven/"
ASSET=BASE+"Data/DA_MapGraph_Zone1"
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets=unreal.EditorAssetLibrary
bel=unreal.BlueprintEditorLibrary
lib=unreal.MapBlueprintMigrationLibrary
def world(): return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
TYPES={"BP_NodeDifficulty_C":unreal.MapNodeType.COMBAT,"BP_NodeBoss_C":unreal.MapNodeType.BOSS,
"BP_NodeCamp_C":unreal.MapNodeType.CAMP,"BP_NodeEvent_C":unreal.MapNodeType.EVENT,
"BP_NodeQuest_C":unreal.MapNodeType.QUEST,"BP_NodeShop_C":unreal.MapNodeType.SHOP}
EXTRA={"Q1":["C3"],"C3":["E3"],"E3":["T2"],"T2":["C4","E4"],"C4":["E5"],"E5":["T3"],
"E4":["E6","C5"],"E6":["C5"],"C5":["E8"],"T3":["E9","E7"],"E7":["C6"],"C6":["E10","T4"],
"E9":["E10"],"E8":["T4"],"E10":["B1"],"T4":["B1"]}
assert levels.load_level(BASE+"MapaIntercativo")
all_actors=actors.get_all_level_actors()
placed=[a for a in all_actors if a.get_class().get_name() in TYPES]
old_managers=[a for a in all_actors if a.get_class().get_name()=="BP_MapManager_C"]
assert len(placed)==30 and len(old_managers)==1
assert not assets.does_asset_exist(ASSET),"Do not overwrite an authored graph"
origin=old_managers[0].get_actor_transform()
ids={a.get_path_name():str(a.get_editor_property("NodeID")) for a in placed}
records=[]
for a in placed:
    pos=a.get_actor_location()
    node_id=ids[a.get_path_name()]
    next_ids=[ids[n.get_path_name()] for n in a.get_editor_property("NextNodes")]
    if not next_ids: next_ids=EXTRA.get(node_id,[])
    difficulty=int(a.get_editor_property("DifficultyLevel")) if a.get_class().get_name()=="BP_NodeDifficulty_C" else max(1,5+math.floor((pos.x-2330)/1000)) if pos.x>=2330 else 4
    records.append((node_id,a.get_class().get_name(),pos.x,difficulty,next_ids,
        unreal.MathLibrary.make_relative_transform(a.get_actor_transform(),origin)))
backup=os.path.join(unreal.Paths.project_saved_dir(),"MapDataAssetMigration")
os.makedirs(backup,exist_ok=True)
for name in ["MapaIntercativo","L_TestPartida"]:
    src=os.path.join(unreal.Paths.project_content_dir(),"GameDesign","Luis","Level_Proto","MapaJuego",name+".umap")
    dest=os.path.join(backup,name+".umap")
    assert not os.path.exists(dest),"Migration backup already exists"
    shutil.copy2(src,dest)

base=assets.duplicate_asset(BASE+"Blueprints/BP_MapNode_Base",OUT+"BP_MapNode_Base_Data")
assert base
lib.clear_legacy_logic(base,True,False)
bel.reparent_blueprint(base,unreal.MapNode)
lib.repair_component_parents(base)
bel.compile_blueprint(base)
assert lib.is_blueprint_valid(base)
assets.save_loaded_asset(base)
classes={}
for name in TYPES:
    bp=assets.duplicate_asset(BASE+"Buttons/"+name[:-2],OUT+name[:-2]+"_Data")
    assert bp
    lib.clear_legacy_logic(bp,True,False)
    bel.reparent_blueprint(bp,bel.generated_class(base))
    lib.repair_component_parents(bp)
    bel.compile_blueprint(bp)
    assert lib.is_blueprint_valid(bp)
    assert assets.save_loaded_asset(bp)
    classes[name]=bel.generated_class(bp)

test_level=unreal.load_asset(BASE+"L_TestPartida")
entries=[]
for node_id,name,x,difficulty,next_ids,transform in records:
    data=unreal.MapNodeData()
    section=unreal.MapSection.FIRST if x<=2330 else unreal.MapSection.SECOND if x<6000 else unreal.MapSection.THIRD
    data.set_editor_properties({"node_id":node_id,"section":section,"node_type":TYPES[name],
        "difficulty_level":difficulty,"next_nodes":next_ids,"level":test_level})
    entry=unreal.MapNodeDefinition()
    entry.set_editor_properties({"data":data,"local_transform":transform,"node_class":classes[name]})
    entries.append(entry)
graph=unreal.AssetToolsHelpers.get_asset_tools().create_asset("DA_MapGraph_Zone1",BASE+"Data",
    unreal.MapGraphDataAsset,unreal.DataAssetFactory())
assert graph
graph.set_editor_properties({"map_id":"Zone1","root_node_id":"Diff1","nodes":entries,
    "default_node_class":bel.generated_class(base)})
error=graph.validate_definition()
assert error is not None,str(error)
assert assets.save_loaded_asset(graph)
manager_bp=bel.create_blueprint_asset_with_parent(OUT+"BP_DataMapManager",unreal.LevelMapManager)
unreal.get_default_object(bel.generated_class(manager_bp)).set_editor_property("graph_definition",graph)
bel.compile_blueprint(manager_bp)
assets.save_loaded_asset(manager_bp)
manager=actors.spawn_actor_from_class(bel.generated_class(manager_bp),origin.translation,origin.rotation.rotator())
manager.set_actor_transform(origin,False,False)
manager.set_editor_property("graph_definition",graph)
manager.set_actor_label("BP_DataMapManager")
# Remove legacy level-script interaction; the pawn retains camera dragging.
lib.clear_level_logic(world())
for a in placed+old_managers:
    assert actors.destroy_actor(a)
mode=bel.create_blueprint_asset_with_parent(OUT+"BP_DataMapGameMode",unreal.GameModeBase)
unreal.get_default_object(bel.generated_class(mode)).set_editor_property("default_pawn_class",
    bel.generated_class(unreal.load_asset(BASE+"Blueprints/Character_Map")))
bel.compile_blueprint(mode)
assets.save_loaded_asset(mode)
world().get_world_settings().set_editor_property("default_game_mode",bel.generated_class(mode))
for a in actors.get_all_level_actors():
    s=a.get_actor_scale3d()
    if 0.0 in (s.x,s.y,s.z): a.set_actor_scale3d(unreal.Vector(s.x or 0.001,s.y or 0.001,s.z or 0.001))
assert levels.save_current_level()

# Keep the prototype Space -> complete -> return flow on the native subsystem.
assert levels.load_level(BASE+"L_TestPartida")
lib.clear_level_logic(world())
exit_actor=actors.spawn_actor_from_class(unreal.MapEncounterExit,unreal.Vector())
exit_actor.set_editor_property("return_map",unreal.load_asset(BASE+"MapaIntercativo"))
world().get_world_settings().set_editor_property("default_game_mode",unreal.GameModeBase)
assert levels.save_current_level()
unreal.log("MAP_DATA_ASSET_MIGRATION_COMPLETE: 30 definitions, 3 sections, native visual variants")
