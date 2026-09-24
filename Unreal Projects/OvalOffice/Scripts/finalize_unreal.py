import unreal as u,json,traceback
from pathlib import Path
P=Path(u.Paths.project_dir());A=u.get_editor_subsystem(u.EditorAssetSubsystem);L=u.get_editor_subsystem(u.LevelEditorSubsystem);E=u.get_editor_subsystem(u.UnrealEditorSubsystem);AS=u.get_editor_subsystem(u.EditorActorSubsystem)
main='/Game/OvalOffice/Maps/OvalOffice';cut='/Game/OvalOffice/Maps/OvalOffice_Cutaway'
try:
 L.load_level(main)
 if not A.does_asset_exist(cut):assert A.duplicate_asset(main,cut)
 L.load_level(cut)
 hidden={'OO_Architecture_Ceiling','OO_Architecture_Cornice','OO_Architecture_Panelling','OO_Walls_Front','OO_Garden panorama'}
 for a in list(AS.get_all_level_actors()):
  if a.get_actor_label() in hidden:AS.destroy_actor(a)
 E.set_level_viewport_camera_info(u.Vector(-1200,900,780),u.MathLibrary.find_look_at_rotation(u.Vector(-1200,900,780),u.Vector(0,0,160)));L.save_current_level()
 L.load_level(main);E.set_level_viewport_camera_info(u.Vector(-465,-24,188),u.MathLibrary.find_look_at_rotation(u.Vector(-465,-24,188),u.Vector(250,0,159)));L.save_current_level()
 d=json.loads((P/'SourceAssets'/'manifest.json').read_text());meshes=A.list_assets('/Game/OvalOffice/Meshes',recursive=True,include_folder=False)
 report={'status':'complete','meshes':len(meshes),'material_instances':len(d['materials']),'triangles':sum(x['triangles'] for x in d['groups']),'maps':[main,cut],'source':'Original authored Oval_Office.blend','engine':u.SystemLibrary.get_engine_version(),'actor_count':len(AS.get_all_level_actors())}
 assert len(meshes)==31;assert A.does_asset_exist(main) and A.does_asset_exist(cut)
 (P/'Saved'/'import_complete.json').write_text(json.dumps(report,indent=2));u.log('OVAL_OFFICE_FINISHED '+str(report))
except Exception:
 (P/'Saved'/'finalize_error.txt').write_text(traceback.format_exc());raise
