import unreal as u,json,traceback,re
from pathlib import Path
P=Path(u.Paths.project_dir());S=P/'SourceAssets'/'AirForceOne';D=json.loads((S/'manifest.json').read_text())
BASE='/Game/AirForceOne';MAP=BASE+'/Maps/AirForceOne';REPORT=[]
tools=u.AssetToolsHelpers.get_asset_tools();assets=u.get_editor_subsystem(u.EditorAssetSubsystem);actors=u.get_editor_subsystem(u.EditorActorSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem);ml=u.MaterialEditingLibrary
def note(*v):
 msg=' '.join(map(str,v));REPORT.append(msg);(P/'Saved'/'air_force_one_build.txt').write_text('\n'.join(REPORT));u.log('AF1 '+msg)
def asset(cls,name,folder,factory):return u.load_asset(BASE+'/'+folder+'/'+name) or tools.create_asset(name,BASE+'/'+folder,cls,factory)
def texture(filename,normal=False):
 name='T_'+Path(filename).stem;dest=BASE+'/Textures/'+name;t=u.load_asset(dest)
 if not t:
  task=u.AssetImportTask()
  for k,v in [('filename',str(S/'Textures'/filename)),('destination_path',BASE+'/Textures'),('destination_name',name),('automated',True),('save',True),('factory',u.TextureFactory())]:task.set_editor_property(k,v)
  tools.import_asset_tasks([task]);t=u.load_asset(dest)
 if not t:raise RuntimeError('Texture import failed '+filename)
 if normal:
  t.set_editor_property('srgb',False);t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);assets.save_loaded_asset(t,False)
 return t
def scalar(m,name,value,prop=None):
 n=ml.create_material_expression(m,u.MaterialExpressionScalarParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',value)
 if prop:ml.connect_material_property(n,'',prop)
 return n
def material(info):
 m=asset(u.Material,'M_'+info['id'],'Materials',u.MaterialFactoryNew());ml.delete_all_material_expressions(m);m.set_editor_property('two_sided',True)
 if info.get('texture'):
  t=ml.create_material_expression(m,u.MaterialExpressionTextureSample);t.set_editor_property('texture',texture(info['texture']));ml.connect_material_property(t,'RGB',u.MaterialProperty.MP_BASE_COLOR)
 else:
  c=ml.create_material_expression(m,u.MaterialExpressionConstant3Vector);c.set_editor_property('constant',u.LinearColor(*info['color'],1));ml.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
 scalar(m,'Roughness',info['roughness'],u.MaterialProperty.MP_ROUGHNESS);scalar(m,'Metallic',info['metallic'],u.MaterialProperty.MP_METALLIC)
 if info.get('normal'):
  n=ml.create_material_expression(m,u.MaterialExpressionTextureSample);n.set_editor_property('texture',texture(info['normal'],True));n.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
  flat=ml.create_material_expression(m,u.MaterialExpressionConstant3Vector);flat.set_editor_property('constant',u.LinearColor(0,0,1,1))
  lerp=ml.create_material_expression(m,u.MaterialExpressionLinearInterpolate);lerp.set_editor_property('const_alpha',info['normal_strength']);ml.connect_material_expressions(flat,'',lerp,'A');ml.connect_material_expressions(n,'RGB',lerp,'B');ml.connect_material_property(lerp,'',u.MaterialProperty.MP_NORMAL)
 if info['glass']:
  m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT);scalar(m,'Opacity',.075,u.MaterialProperty.MP_OPACITY)
 if info['name']=='Aircraft window cloud panorama':
  m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
  multiply=ml.create_material_expression(m,u.MaterialExpressionMultiply);multiply.set_editor_property('const_b',24.0);ml.connect_material_expressions(t,'RGB',multiply,'A');ml.connect_material_property(multiply,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
 elif info['name']=='Luminous champagne cove':
  c=ml.create_material_expression(m,u.MaterialExpressionConstant3Vector);c.set_editor_property('constant',u.LinearColor(12,8,4,1));ml.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
 elif 'Luminous' in info['name'] or 'daylight' in info['name'].lower():
  c=ml.create_material_expression(m,u.MaterialExpressionConstant3Vector);c.set_editor_property('constant',u.LinearColor(.6,.72,1,1));ml.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
 ml.recompile_material(m);assets.save_loaded_asset(m,False);return m
def spawn(cls,name,loc,rotation=None,folder='Lighting'):
 a=actors.spawn_actor_from_class(cls,u.Vector(*loc),rotation or u.Rotator());a.set_actor_label('AF1_'+name);a.set_folder_path('Air Force One/'+folder);return a
def look(loc,target):return u.MathLibrary.find_look_at_rotation(u.Vector(*loc),u.Vector(*target))
BP=u.BlueprintEditorLibrary;PIN=u.BlueprintGraphPinLibrary
def call(e,path):
 n=e.add_call_function_node(path)
 if not n:raise RuntimeError('Function unavailable '+path)
 return n
def link(a,ap,b,bp):
 src=a.find_then_pin() if ap=='then' else a.find_output_pin(ap);dst=b.find_execute_pin() if bp=='execute' else b.find_input_pin(bp)
 if not src or not dst or not PIN.try_create_connection(src,dst):raise RuntimeError('Cannot link '+str(a.get_node_title())+'.'+ap+' to '+str(b.get_node_title())+'.'+bp)
def val(n,p,v):
 pin=n.find_input_pin(p)
 if not pin or not PIN.set_pin_value(pin,str(v)):raise RuntimeError('Cannot set '+str(n.get_node_title())+'.'+p+'='+str(v))
def newbp(name,parent):
 f=u.BlueprintFactory();f.set_editor_property('parent_class',parent);return asset(u.Blueprint,name,'Blueprints',f)
def finish(bp):
 ok=BP.compile_blueprint(bp);e=u.BlueprintGraphEditor.get_graph_editor(BP.find_event_graph(bp))
 errors=[str(n.get_node_title()) for n in e.list_nodes_with_errors()];note(bp.get_name(),'compiled',ok,'errors',errors)
 if not ok or errors:raise RuntimeError('Blueprint compilation failed')
 assets.save_loaded_asset(bp,False)
def controls():
 bp=newbp('BP_AF1_Walker',u.Character);e=u.BlueprintGraphEditor.get_graph_editor(BP.find_event_graph(bp))
 e.remove_nodes([n for n in e.list_all_nodes() if n.get_class().get_name()!='K2Node_Event'])
 tick=e.find_event_node('ReceiveTick');begin=e.find_event_node('ReceiveBeginPlay');pc=call(e,'/Script/Engine.GameplayStatics.GetPlayerController')
 prior=tick
 for positive,negative,fn in [('W','S','GetActorForwardVector'),('D','A','GetActorRightVector')]:
  sels=[]
  for key,amount in [(positive,1),(negative,-1)]:
   # FKey imports a bare key name; struct-style '(KeyName=W)' parses as an invalid key.
   down=call(e,'/Script/Engine.PlayerController.IsInputKeyDown');link(pc,'ReturnValue',down,'self');val(down,'Key',key)
   assert str(PIN.get_pin_value(down.find_input_pin('Key')))==key, 'Invalid movement key serialization'
   sel=call(e,'/Script/Engine.KismetMathLibrary.SelectFloat');val(sel,'A',amount);val(sel,'B',0);link(down,'ReturnValue',sel,'bPickA');sels.append(sel)
  add=call(e,'/Script/Engine.KismetMathLibrary.Add_DoubleDouble');link(sels[0],'ReturnValue',add,'A');link(sels[1],'ReturnValue',add,'B')
  direction=call(e,'/Script/Engine.Actor.'+fn);move=call(e,'/Script/Engine.Pawn.AddMovementInput');link(prior,'then',move,'execute');link(direction,'ReturnValue',move,'WorldDirection');link(add,'ReturnValue',move,'ScaleValue');prior=move
 mouse=call(e,'/Script/Engine.PlayerController.GetInputMouseDelta');link(pc,'ReturnValue',mouse,'self')
 for axis,func,scale in [('DeltaX','AddControllerYawInput',.35),('DeltaY','AddControllerPitchInput',-.35)]:
  mul=call(e,'/Script/Engine.KismetMathLibrary.Multiply_DoubleDouble');link(mouse,axis,mul,'A');val(mul,'B',scale)
  turn=call(e,'/Script/Engine.Pawn.'+func);link(prior,'then',turn,'execute');link(mul,'ReturnValue',turn,'Val');prior=turn
 intro=call(e,'/Script/Engine.KismetSystemLibrary.PrintString');val(intro,'InString','AIR FORCE ONE  |  W A S D walk  |  Mouse look  |  Esc exit');val(intro,'Duration',12);link(begin,'then',intro,'execute')
 esc=e.create_node_from_name('Input|KeyboardEvents|Escape',u.Vector2D(0,0),[]);quit=call(e,'/Script/Engine.KismetSystemLibrary.QuitGame');link(esc,'Pressed',quit,'execute');link(pc,'ReturnValue',quit,'SpecificPlayer')
 finish(bp)
 cls=u.load_class(None,BASE+'/Blueprints/BP_AF1_Walker.BP_AF1_Walker_C');cdo=u.get_default_object(cls)
 cdo.set_editor_property('base_eye_height',70);cdo.set_editor_property('use_controller_rotation_yaw',True)
 cap=cdo.get_component_by_class(u.CapsuleComponent);cap.set_capsule_size(34,90)
 movement=cdo.get_component_by_class(u.CharacterMovementComponent);movement.set_editor_property('max_walk_speed',180);movement.set_editor_property('max_step_height',15)
 assets.save_loaded_asset(bp,False)
 gm=newbp('BP_AF1_GameMode',u.GameModeBase);finish(gm);gc=u.load_class(None,BASE+'/Blueprints/BP_AF1_GameMode.BP_AF1_GameMode_C');u.get_default_object(gc).set_editor_property('default_pawn_class',cls);assets.save_loaded_asset(gm,False)
 return gc
try:
 note('Starting Unreal Air Force One build',u.SystemLibrary.get_engine_version())
 mats={i['id']:material(i) for i in D['materials']};note('Materials',len(mats))
 meshes={}
 for entry in D['groups']:
  name=entry['name'];dest=BASE+'/Meshes/SM_'+name;mesh=u.load_asset(dest)
  if True: # Reimport source meshes so revised geometry and coordinate conversion stay in sync.
   options=u.FbxImportUI()
   for k,v in [('import_mesh',True),('import_materials',False),('import_textures',False),('import_as_skeletal',False),('mesh_type_to_import',u.FBXImportType.FBXIT_STATIC_MESH),('automated_import_should_detect_type',False)]:options.set_editor_property(k,v)
   opt=options.static_mesh_import_data
   for k,v in [('combine_meshes',True),('auto_generate_collision',False),('generate_lightmap_u_vs',False),('convert_scene',False),('convert_scene_unit',False)]:opt.set_editor_property(k,v)
   opt.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
   task=u.AssetImportTask()
   for k,v in [('filename',str(S/'Meshes'/(name+'.obj'))),('destination_path',BASE+'/Meshes'),('destination_name','SM_'+name),('replace_existing',True),('automated',True),('save',True),('factory',u.FbxFactory()),('options',options)]:task.set_editor_property(k,v)
   tools.import_asset_tasks([task]);mesh=u.load_asset(dest)
  if not mesh:raise RuntimeError('Import failed '+name)
  for i,slot in enumerate(mesh.get_editor_property('static_materials')):
   key=str(slot.get_editor_property('material_slot_name'))
   # Reimport retains old user-facing slot names when newly added materials shift
   # the export indices. Resolve those retained slots by the stable name suffix.
   if key not in mats:
    suffix=re.sub(r'^AF\d+_','',key);matches=[k for k in mats if re.sub(r'^AF\d+_','',k)==suffix]
    if len(matches)!=1:raise RuntimeError('Unresolved material slot '+key)
    key=matches[0]
   mesh.set_material(i,mats[key])
  body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE);body.set_editor_property('double_sided_geometry',True)
  assets.save_loaded_asset(mesh,False);meshes[name]=mesh;note('Imported',name,entry['triangles'],'bounds',mesh.get_bounding_box())
 gm=controls();note('First person controls ready')
 if assets.does_asset_exist(MAP):levels.load_level(MAP)
 else:levels.new_level(MAP)
 for a in list(actors.get_all_level_actors()):
  if a.get_actor_label().startswith('AF1_'):actors.destroy_actor(a)
 for name,mesh in meshes.items():
  a=spawn(u.StaticMeshActor,name,(0,0,0),folder='Cabin');a.static_mesh_component.set_static_mesh(mesh);a.static_mesh_component.set_collision_profile_name('BlockAll');a.set_actor_enable_collision(name!='Window_Glass')
  if name=='Window_Glass':a.static_mesh_component.set_cast_shadow(False)
 # Soft daylight and warm ceiling pools. Broad non-shadow fills stand in for bounced light on DX11.
 def lamp(name,loc,target,intensity,color,size=100,shadow=True):
  a=spawn(u.RectLight,name,loc,look(loc,target));c=a.get_component_by_class(u.RectLightComponent);c.set_mobility(u.ComponentMobility.MOVABLE);c.set_intensity(intensity);c.set_light_color(u.LinearColor(*color,1));c.set_editor_property('source_width',size);c.set_editor_property('source_height',size);c.set_editor_property('attenuation_radius',800);c.set_cast_shadows(shadow);return c
 for i,y in enumerate([95,245,650,830,1030,1210]):lamp('Window daylight '+str(i),(y,-320,150),(y,40,100),600,(.80,.9,1),100)
 for i,y in enumerate([130,340,630,850,1100]):lamp('Cabin ceiling '+str(i),(y,-50,230),(y,-50,0),450,(1,.88,.72),130,False)
 for i,y in enumerate([110,440,850,1190]):lamp('Hall light '+str(i),(y,276,224),(y,276,0),170,(1,.89,.75),65,False)
 for i,y in enumerate([118,329,540,751,962,1173]):lamp('Passage window daylight '+str(i),(y,339,156),(y,210,125),95,(.80,.9,1),55,False)
 for i,y in enumerate([100,350,680,1050]):lamp('Cabin soft bounce '+str(i),(y,155,150),(y,-280,140),180,(1,.93,.83),150,False)
 # Pale sky outside the windows, kept clear of the window geometry.
 sky=asset(u.Material,'M_OutsideSky','Materials',u.MaterialFactoryNew());ml.delete_all_material_expressions(sky);sky.set_editor_property('two_sided',True);sky.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
 c=ml.create_material_expression(sky,u.MaterialExpressionConstant3Vector);c.set_editor_property('constant',u.LinearColor(20,28,40,1));ml.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR);ml.recompile_material(sky);assets.save_loaded_asset(sky,False)
 a=spawn(u.StaticMeshActor,'Sky beyond windows',(650,-500,150),folder='Exterior');a.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'));a.static_mesh_component.set_material(0,sky);a.set_actor_scale3d(u.Vector(16,.02,10));a.static_mesh_component.set_cast_shadow(False);a.set_actor_enable_collision(False)
 pp=spawn(u.PostProcessVolume,'Interior exposure',(0,0,0));pp.set_editor_property('unbound',True);settings=pp.get_editor_property('settings')
 for k,v in [('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',1.0),('auto_exposure_max_brightness',1.0),('override_auto_exposure_bias',True),('auto_exposure_bias',0.0),('override_motion_blur_amount',True),('motion_blur_amount',0.0),('override_ambient_occlusion_intensity',True),('ambient_occlusion_intensity',.8)]:settings.set_editor_property(k,v)
 settings.set_editor_property('auto_exposure_bias',-4.5)
 pp.set_editor_property('settings',settings)
 spawn(u.PlayerStart,'Player start',(365,95,94),u.Rotator(pitch=0,yaw=-130,roll=0),'Player')
 for label,loc,target in [('Office',(365,95,165),(170,-80,110)),('Hallway',(70,276,165),(1000,276,160)),('Conference',(610,150,165),(1040,-90,115))]:
  a=spawn(u.CameraActor,label,loc,look(loc,target),'Cameras');a.camera_component.set_editor_property('field_of_view',80)
 world=editor.get_editor_world();world.get_world_settings().set_editor_property('default_game_mode',gm)
 editor.set_level_viewport_camera_info(u.Vector(365,95,165),look((365,95,165),(170,-80,110)));actors.clear_actor_selection_set();levels.save_current_level();assets.save_directory(BASE,False,True)
 note('SUCCESS',MAP,'Meshes',len(meshes),'Materials',len(mats))
 (P/'Saved'/'air_force_one_build.json').write_text(json.dumps({'status':'complete','map':MAP,'meshes':len(meshes),'materials':len(mats),'triangles':sum(x['triangles'] for x in D['groups']),'player_capsule_radius_cm':34,'player_capsule_half_height_cm':90,'walking_speed_cm_per_s':180,'collision':'complex static mesh collision','runtime_test':'pending'},indent=2))
except Exception:
 note('ERROR',traceback.format_exc());raise
