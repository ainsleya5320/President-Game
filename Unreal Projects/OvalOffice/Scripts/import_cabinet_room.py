"""Import the authored Blender Cabinet Room and connect the two native levels.
Run inside Unreal: py <path>/Scripts/import_cabinet_room.py
Only actors labelled CR_ or FY_CabinetLink are replaced on repeat runs.
"""
import unreal as u,json,traceback
from pathlib import Path
P=Path(u.Paths.project_dir());S=P/'SourceAssets'/'CabinetRoom';BASE='/Game/CabinetRoom';LOG=[]
a=u.get_editor_subsystem(u.EditorActorSubsystem);assets=u.get_editor_subsystem(u.EditorAssetSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem);tools=u.AssetToolsHelpers.get_asset_tools();ml=u.MaterialEditingLibrary

def note(s):
 LOG.append(s);(P/'Saved'/'CabinetImport.log').write_text('\n'.join(LOG));u.log(s)
def spawn(cls,name,loc,rot=None):
 o=a.spawn_actor_from_class(cls,u.Vector(*loc),rot or u.Rotator());o.set_actor_label(name);o.set_folder_path('Cabinet Room');return o
def look(loc,target):return u.MathLibrary.find_look_at_rotation(u.Vector(*loc),u.Vector(*target))
def texture(filename):
 dest=BASE+'/Textures/T_'+Path(filename).stem;t=u.load_asset(dest)
 if not t:
  task=u.AssetImportTask()
  for k,v in dict(filename=str(S/'Textures'/filename),destination_path=BASE+'/Textures',destination_name='T_'+Path(filename).stem,automated=True,save=True,factory=u.TextureFactory()).items():task.set_editor_property(k,v)
  tools.import_asset_tasks([task]);t=u.load_asset(dest)
 if not t:raise RuntimeError(filename)
 return t
def master(name,textured=False):
 m=u.load_asset(BASE+'/Materials/'+name) or tools.create_asset(name,BASE+'/Materials',u.Material,u.MaterialFactoryNew())
 ml.delete_all_material_expressions(m);m.set_editor_property('two_sided',True)
 tint=ml.create_material_expression(m,u.MaterialExpressionVectorParameter);tint.set_editor_property('parameter_name','Tint');tint.set_editor_property('default_value',u.LinearColor(1,1,1,1));output=tint
 if textured:
  tex=ml.create_material_expression(m,u.MaterialExpressionTextureSampleParameter2D);tex.set_editor_property('parameter_name','Albedo');tex.set_editor_property('texture',texture('washington_portrait.png'))
  mul=ml.create_material_expression(m,u.MaterialExpressionMultiply)
  assert ml.connect_material_expressions(tex,'RGB',mul,'A')
  assert ml.connect_material_expressions(tint,'',mul,'B')
  output=mul
 assert ml.connect_material_property(output,'',u.MaterialProperty.MP_BASE_COLOR)
 for param,value,prop in [('Roughness',.5,u.MaterialProperty.MP_ROUGHNESS),('Metallic',0,u.MaterialProperty.MP_METALLIC)]:
  node=ml.create_material_expression(m,u.MaterialExpressionScalarParameter);node.set_editor_property('parameter_name',param);node.set_editor_property('default_value',value);assert ml.connect_material_property(node,'',prop)
 ml.layout_material_expressions(m);ml.recompile_material(m);assets.save_loaded_asset(m,False)
 return m

def rect(name,loc,target,power,width,height,color=(1,.94,.84),shadows=True):
 o=spawn(u.RectLight,'CR_'+name,loc,look(loc,target));c=o.get_component_by_class(u.RectLightComponent);c.set_mobility(u.ComponentMobility.MOVABLE);c.set_intensity(power/16.0);c.set_light_color(u.LinearColor(*color,1));c.set_editor_property('source_width',width);c.set_editor_property('source_height',height);c.set_editor_property('attenuation_radius',1800);c.set_cast_shadows(shadows)
def box(x1,y1,x2,y2):return u.Box2D(min=u.Vector2D(x1,y1),max=u.Vector2D(x2,y2))
def interaction(pos,prompt,action,target,radius=110):
 i=u.FourYearsRoomInteraction();i.position=u.Vector2D(*pos);i.prompt=prompt;i.action=action;i.target=target;i.radius=radius;return i
try:
 u.SystemLibrary.execute_console_command(editor.get_editor_world(),'Interchange.FeatureFlags.Import.OBJ 0')
 D=json.loads((S/'manifest.json').read_text(encoding='utf-8-sig'));mats={}
 plain_master=master('M_CabinetSurface');textured_master=master('M_CabinetTextured',True)
 for info in D['materials']:
  name='MI_'+info['id'];m=u.load_asset(BASE+'/Materials/'+name) or tools.create_asset(name,BASE+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
  parent=textured_master if info['texture'] else plain_master
  ml.set_material_instance_parent(m,parent);ml.set_material_instance_vector_parameter_value(m,'Tint',u.LinearColor(*info['color'],1));ml.set_material_instance_scalar_parameter_value(m,'Roughness',info['roughness']);ml.set_material_instance_scalar_parameter_value(m,'Metallic',info['metallic'])
  if info['texture']:ml.set_material_instance_texture_parameter_value(m,'Albedo',texture(info['texture']))
  assets.save_loaded_asset(m,False);mats[info['id']]=m
 note('Materials imported')
 meshes={}
 for group in D['groups']:
  name=group['name'];options=u.FbxImportUI()
  for k,v in dict(import_mesh=True,import_materials=False,import_textures=False,import_as_skeletal=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,automated_import_should_detect_type=False).items():options.set_editor_property(k,v)
  for k,v in dict(combine_meshes=True,auto_generate_collision=False,generate_lightmap_u_vs=False,convert_scene=False,convert_scene_unit=False,normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS).items():options.static_mesh_import_data.set_editor_property(k,v)
  task=u.AssetImportTask()
  for k,v in dict(filename=str(S/'Meshes'/(name+'.obj')),destination_path=BASE+'/Meshes',destination_name='SM_'+name,replace_existing=True,automated=True,save=True,factory=u.FbxFactory(),options=options).items():task.set_editor_property(k,v)
  tools.import_asset_tasks([task]);mesh=u.load_asset(BASE+'/Meshes/SM_'+name)
  if not mesh:raise RuntimeError('Missing '+name)
  # FBX's OBJ reader labels the initial usemtl block defaultMat. Recover its authored name.
  first_material=next(line.split()[1] for line in (S/'Meshes'/(name+'.obj')).open() if line.startswith('usemtl '))
  assigned=set()
  for i,slot in enumerate(mesh.static_materials):
   key=str(slot.material_slot_name)
   if key=='defaultMat':key=first_material
   mesh.set_material(i,mats[key]);assigned.add(key)
  if assigned!=set(group['materials']):raise RuntimeError('Material slot mismatch in '+name+': '+str(assigned))
  assets.save_loaded_asset(mesh,False);meshes[name]=mesh;note('Imported '+name+' ('+str(group['triangles'])+' triangles)')
 map_path=BASE+'/Maps/CabinetRoom'
 if assets.does_asset_exist(map_path):levels.load_level(map_path)
 else:levels.new_level(map_path)
 for actor in list(a.get_all_level_actors()):
  if actor.get_actor_label().startswith('CR_'):a.destroy_actor(actor)
 for name,mesh in meshes.items():
  o=spawn(u.StaticMeshActor,'CR_'+name,(0,0,0));o.static_mesh_component.set_static_mesh(mesh);o.set_actor_enable_collision(False)
 # Panorama fills beyond the modelled rose beds and trees.
 env=spawn(u.StaticMeshActor,'CR_Garden panorama',(0,0,0));env.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Sphere'));env.static_mesh_component.set_material(0,u.load_asset('/Game/OvalOffice/Materials/M_GardenPanorama'));env.static_mesh_component.set_cast_shadow(False);env.set_actor_enable_collision(False);env.set_actor_scale3d(u.Vector(180,180,180))
 sky=spawn(u.SkyLight,'CR_Daylight ambient',(0,0,700));sc=sky.get_component_by_class(u.SkyLightComponent);sc.set_mobility(u.ComponentMobility.MOVABLE);sc.set_intensity(.9);sc.set_editor_property('source_type',u.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP);sc.set_editor_property('cubemap',u.load_asset('/Game/OvalOffice/Textures/T_garden'));sc.set_editor_property('lower_hemisphere_is_black',False)
 sun=spawn(u.DirectionalLight,'CR_Afternoon sun',(-300,900,1100),look((-300,900,1100),(100,0,0)));sun.get_component_by_class(u.DirectionalLightComponent).set_mobility(u.ComponentMobility.MOVABLE);sun.get_component_by_class(u.DirectionalLightComponent).set_intensity(2.0)
 for x in [-510,-255,0,255,510]:rect('Garden light '+str(x),(x,365,240),(x,-80,110),1600,170,235,(.91,.96,1))
 for x in [-370,0,370]:rect('Pendant fill '+str(x),(x,0,355),(x,0,30),1400,240,200,(1,.90,.74),False)
 rect('Portrait wall bounce',(0,-310,270),(0,0,130),900,620,120,(1,.94,.85),False)
 rect('Vestibule light',(-740,260,340),(-740,260,0),450,120,120,shadows=False)
 pp=spawn(u.PostProcessVolume,'CR_Exposure',(0,0,0));pp.set_editor_property('unbound',True);settings=pp.settings
 for k,v in dict(override_auto_exposure_min_brightness=True,override_auto_exposure_max_brightness=True,auto_exposure_min_brightness=1.,auto_exposure_max_brightness=1.,override_auto_exposure_bias=True,auto_exposure_bias=-2.,override_motion_blur_amount=True,motion_blur_amount=0.).items():settings.set_editor_property(k,v)
 pp.settings=settings
 room=spawn(u.FourYearsRoom,'CR_Gameplay',(0,0,0));room.room_label='THE CABINET ROOM'
 room.walkable_areas=[box(-620,-345,620,345),box(-790,170,-600,345)]
 room.obstacles=[box(-515,-224,515,224),box(-150,-380,150,-262),box(580,-125,650,125)]
 room.arrival_position=u.Vector(-575,280,165);room.arrival_yaw=-10
 room.interactions=[interaction((-780,260),'Press E to return to the Oval Office','travel','/Game/OvalOffice/Maps/OvalOffice',105),interaction((-200,278),'Press E to speak with Maya Chen','adviser','chen',100),interaction((200,278),'Press E to speak with Elena Ruiz','adviser','ruiz',100),interaction((-200,-278),'Press E to speak with Daniel Brooks','adviser','brooks',100),interaction((200,-278),'Press E to speak with Samira Bell','adviser','bell',100),interaction((0,278),'Press E at the presidential chair for your quarterly briefing','briefing','',75)]
 hero=(-575,280,165);target=(260,0,140)
 spawn(u.PlayerStart,'CR_Start',hero,look(hero,target));cam=spawn(u.CameraActor,'CR_Visitor view',hero,look(hero,target));cam.camera_component.field_of_view=78
 editor.get_editor_world().get_world_settings().set_editor_property('default_game_mode',u.FourYearsGameMode)
 editor.set_level_viewport_camera_info(u.Vector(*hero),look(hero,target));levels.save_current_level();assets.save_directory(BASE,False,True)
 note('Cabinet Room saved')
 # Add a labelled, reversible portal at the existing Oval Office east door.
 levels.load_level('/Game/OvalOffice/Maps/OvalOffice')
 for actor in list(a.get_all_level_actors()):
  if actor.get_actor_label().startswith('FY_CabinetLink'):a.destroy_actor(actor)
 link=spawn(u.FourYearsRoom,'FY_CabinetLink',(0,0,0));link.room_label='THE OVAL OFFICE';link.arrival_position=u.Vector(70,310,165);link.arrival_yaw=-90
 link.interactions=[interaction((30,370),'Press E at this door to visit the Cabinet Room','travel',map_path,105)]
 sign=spawn(u.TextRenderActor,'FY_CabinetLink_Sign',(20,409,224),u.Rotator(pitch=0,yaw=-90,roll=0));tc=sign.get_component_by_class(u.TextRenderComponent);tc.set_text('CABINET ROOM');tc.set_world_size(9);tc.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER);tc.set_text_render_color(u.Color(215,179,102,255))
 editor.get_editor_world().get_world_settings().set_editor_property('default_game_mode',u.FourYearsGameMode);levels.save_current_level()
 levels.load_level(map_path);editor.set_level_viewport_camera_info(u.Vector(*hero),look(hero,target));a.clear_actor_selection_set()
 note('SUCCESS: connected Oval Office and Cabinet Room; 4 adviser seats; term survives level travel')
 (P/'Saved'/'CabinetImport.json').write_text(json.dumps({'status':'complete','map':map_path,'meshes':len(meshes),'triangles':sum(g['triangles'] for g in D['groups'])},indent=2))
except Exception:
 note(traceback.format_exc());raise
