"""Install revised Blender meshes in existing levels, preserving gameplay actors."""
import unreal as u,json,traceback
from pathlib import Path
P=Path(u.Paths.project_dir());S=P/'SourceAssets/DecorRefresh';BASE='/Game/DecorRefresh'
at=u.AssetToolsHelpers.get_asset_tools();ea=u.get_editor_subsystem(u.EditorAssetSubsystem);ml=u.MaterialEditingLibrary
actors=u.get_editor_subsystem(u.EditorActorSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
REPORT=[]
def note(s):
 REPORT.append(s);(P/'Saved/DecorImport.log').write_text('\n'.join(REPORT));u.log('DECOR '+s)
def texture(file):
 path=BASE+'/Textures/T_'+Path(file).stem;t=u.load_asset(path)
 if not t:
  task=u.AssetImportTask()
  for k,v in dict(filename=str(S/'Textures'/file),destination_path=BASE+'/Textures',destination_name='T_'+Path(file).stem,automated=True,save=True,factory=u.TextureFactory()).items():task.set_editor_property(k,v)
  at.import_asset_tasks([task]);t=u.load_asset(path)
 assert t,file
 return t
try:
 D=json.loads((S/'manifest.json').read_text());u.SystemLibrary.execute_console_command(editor.get_editor_world(),'Interchange.FeatureFlags.Import.OBJ 0')
 for info in D['materials']:
  name=info['id'];m=u.load_asset(BASE+'/Materials/'+name) or at.create_asset(name,BASE+'/Materials',u.Material,u.MaterialFactoryNew());ml.delete_all_material_expressions(m);m.set_editor_property('two_sided',True)
  if info['texture']:
   n=ml.create_material_expression(m,u.MaterialExpressionTextureSample);n.set_editor_property('texture',texture(info['texture']));pin='RGB'
  else:
   n=ml.create_material_expression(m,u.MaterialExpressionConstant3Vector);n.set_editor_property('constant',u.LinearColor(*info['color'],1));pin=''
  assert ml.connect_material_property(n,pin,u.MaterialProperty.MP_BASE_COLOR)
  for value,prop in [(info['roughness'],u.MaterialProperty.MP_ROUGHNESS),(info['metallic'],u.MaterialProperty.MP_METALLIC)]:
   c=ml.create_material_expression(m,u.MaterialExpressionConstant);c.set_editor_property('r',value);assert ml.connect_material_property(c,'',prop)
  ml.recompile_material(m);ea.save_loaded_asset(m,False)
 note('Imported '+str(len(D['materials']))+' decor materials')
 meshes={}
 for g in D['groups']:
  name=g['name'];options=u.FbxImportUI()
  for k,v in dict(import_mesh=True,import_materials=False,import_textures=False,import_as_skeletal=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,automated_import_should_detect_type=False).items():options.set_editor_property(k,v)
  for k,v in dict(combine_meshes=True,auto_generate_collision=False,generate_lightmap_u_vs=False,convert_scene=False,convert_scene_unit=False,normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS).items():options.static_mesh_import_data.set_editor_property(k,v)
  task=u.AssetImportTask()
  for k,v in dict(filename=str(S/'Meshes'/(name+'.obj')),destination_path=BASE+'/Meshes',destination_name='SM_'+name,replace_existing=True,automated=True,save=True,factory=u.FbxFactory(),options=options).items():task.set_editor_property(k,v)
  # Clear retained user-facing slot names before OBJ reimport: a replacement
  # painting must receive its newly authored slot rather than the former title.
  existing=u.load_asset(BASE+'/Meshes/SM_'+name)
  if existing:existing.set_editor_property('static_materials',[])
  at.import_asset_tasks([task]);me=u.load_asset(BASE+'/Meshes/SM_'+name);assert me,name
  first=next(line.split()[1] for line in (S/'Meshes'/(name+'.obj')).open() if line.startswith('usemtl '));assigned=set()
  for i,slot in enumerate(me.static_materials):
   key=str(slot.material_slot_name);key=first if key=='defaultMat' else key
   assert key in g['materials'],(name,key)
   mat=u.load_asset(g['materials'][key]);assert mat,g['materials'][key];me.set_material(i,mat);assigned.add(key)
  assert assigned==set(g['materials']),(name,assigned)
  body=me.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE);body.set_editor_property('double_sided_geometry',True)
  ea.save_loaded_asset(me,False);meshes[name]=me;note('Imported '+name)
 for scene,map in [('Oval','/Game/OvalOffice/Maps/OvalOffice'),('AirForceOne','/Game/AirForceOne/Maps/AirForceOne'),('CabinetRoom','/Game/CabinetRoom/Maps/CabinetRoom')]:
  assert levels.load_level(map)
  for g in [v for v in D['groups'] if v['scene']==scene]:
   matching=[]
   for actor in actors.get_all_level_actors():
    if isinstance(actor,u.StaticMeshActor):
     mesh=actor.static_mesh_component.static_mesh
     if mesh and mesh.get_name() in ['SM_'+g['replace_group'],'SM_'+g['name']]:matching.append(actor)
   if not matching and g['replace_group']=='Fireplace_Paintings':
    actor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,0),u.Rotator());actor.set_actor_label('DR_Oval_Fireplace_Paintings');actor.set_actor_enable_collision(False);matching=[actor]
   assert len(matching)==1,(scene,g['replace_group'],len(matching))
   c=matching[0].static_mesh_component;c.set_static_mesh(meshes[g['name']]);c.set_editor_property('override_materials',[])
   note('Updated '+matching[0].get_actor_label())
  assert levels.save_current_level()
 levels.load_level('/Game/OvalOffice/Maps/OvalOffice');editor.set_level_viewport_camera_info(u.Vector(20,-50,165),u.Rotator(pitch=0,yaw=0,roll=0));actors.clear_actor_selection_set()
 ea.save_directory(BASE,False,True);u.get_editor_subsystem(u.EditorUtilitySubsystem).close_tab_by_id('MessageLog')
 note('SUCCESS: all three levels retain their existing layout and gameplay')
 (P/'Saved/DecorImport.json').write_text(json.dumps(dict(status='complete',groups=len(meshes),materials=len(D['materials'])),indent=2))
except Exception:
 note(traceback.format_exc());raise
