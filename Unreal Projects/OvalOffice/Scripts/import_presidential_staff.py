"""Import the Blender staff cast without changing room maps or gameplay saves."""
import unreal as u, json, traceback
from pathlib import Path
P=Path(u.Paths.project_dir());S=P/globals().get('SOURCE_ASSET_DIRECTORY','SourceAssets/PresidentialStaff');BASE=globals().get('ASSET_BASE','/Game/PresidentialStaff')
REPORT=globals().get('REPORT_PREFIX','StaffImport')
at=u.AssetToolsHelpers.get_asset_tools();ea=u.get_editor_subsystem(u.EditorAssetSubsystem);ml=u.MaterialEditingLibrary
report=[]
def note(s):
 report.append(s);(P/'Saved'/(REPORT+'.log')).write_text('\n'.join(report));u.log(REPORT+' '+s)
def texture(file,normal=False):
 path=BASE+'/Textures/T_'+Path(file).stem;t=u.load_asset(path)
 if not t:
  task=u.AssetImportTask()
  for k,v in dict(filename=str(S/'Textures'/file),destination_path=BASE+'/Textures',destination_name='T_'+Path(file).stem,automated=True,save=True,factory=u.TextureFactory()).items():task.set_editor_property(k,v)
  at.import_asset_tasks([task]);t=u.load_asset(path)
 assert t,file
 if normal:
  t.set_editor_property('srgb',False);t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
 t.set_editor_property('max_texture_size',2048);ea.save_loaded_asset(t,False);return t
def constant(m,value):
 n=ml.create_material_expression(m,u.MaterialExpressionConstant);n.set_editor_property('r',value);return n
def color(m,value):
 n=ml.create_material_expression(m,u.MaterialExpressionConstant3Vector);n.set_editor_property('constant',u.LinearColor(*value,1));return n
try:
 d=json.loads((S/'manifest.json').read_text())
 selected=globals().get('STAFF_IDS',[])
 if selected:
  d['groups']=[g for g in d['groups'] if g['name'].split('_')[0] in selected]
  needed={key for g in d['groups'] for key in g['materials']}
  d['materials']=[m for m in d['materials'] if m['id'] in needed]
  d['characters']=[c for c in d['characters'] if c['id'] in selected]
 u.SystemLibrary.execute_console_command(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),'Interchange.FeatureFlags.Import.OBJ 0')
 for info in d['materials']:
  if info['id']=='ST_Studio':continue
  name=info['id'];m=u.load_asset(BASE+'/Materials/'+name) or at.create_asset(name,BASE+'/Materials',u.Material,u.MaterialFactoryNew())
  ml.delete_all_material_expressions(m);m.set_editor_property('two_sided',info.get('two_sided',info['masked']))
  m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT if 'opacity' in info else u.BlendMode.BLEND_MASKED if info['masked'] else u.BlendMode.BLEND_OPAQUE)
  if 'opacity' in info:
   m.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
   ml.connect_material_property(constant(m,info['opacity']),'',u.MaterialProperty.MP_OPACITY)
  if info['masked']:m.set_editor_property('opacity_mask_clip_value',.36)
  if info['texture']:
   n=ml.create_material_expression(m,u.MaterialExpressionTextureSample);n.set_editor_property('texture',texture(info['texture']));pin='RGB'
   if 'mip' in info:
    tex=n.get_editor_property('texture');tex.set_editor_property('power_of_two_mode',u.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO);tex.set_editor_property('mip_gen_settings',u.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE);ea.save_loaded_asset(tex,False)
    n.set_editor_property('mip_value_mode',u.TextureMipValueMode.TMVM_MIP_LEVEL);n.set_editor_property('const_mip_value',info['mip'])
   if info['masked']:ml.connect_material_property(n,'A',u.MaterialProperty.MP_OPACITY_MASK)
   if info['color']!=[1,1,1]:
    tint=color(m,info['color']);mul=ml.create_material_expression(m,u.MaterialExpressionMultiply);ml.connect_material_expressions(n,'RGB',mul,'A');ml.connect_material_expressions(tint,'',mul,'B');n=mul;pin=''
  else:n=color(m,info['color']);pin=''
  assert ml.connect_material_property(n,pin,u.MaterialProperty.MP_BASE_COLOR)
  if info.get('emission',0):
   gain=ml.create_material_expression(m,u.MaterialExpressionMultiply);gain.set_editor_property('const_b',info['emission']);ml.connect_material_expressions(n,pin,gain,'A');ml.connect_material_property(gain,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
  if info.get('metallic',0):ml.connect_material_property(constant(m,info['metallic']),'',u.MaterialProperty.MP_METALLIC)
  for val,prop in [(info['roughness'],u.MaterialProperty.MP_ROUGHNESS),(.24,u.MaterialProperty.MP_SPECULAR)]:ml.connect_material_property(constant(m,val),'',prop)
  if info['normal']:
   n=ml.create_material_expression(m,u.MaterialExpressionTextureSample);n.set_editor_property('texture',texture(info['normal'],True));n.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
   if info.get('normal_strength',1)!=1:
    blend=ml.create_material_expression(m,u.MaterialExpressionLinearInterpolate);blend.set_editor_property('const_alpha',info['normal_strength']);ml.connect_material_expressions(color(m,(0,0,1)),'',blend,'A');ml.connect_material_expressions(n,'RGB',blend,'B');ml.connect_material_property(blend,'',u.MaterialProperty.MP_NORMAL)
   else:ml.connect_material_property(n,'RGB',u.MaterialProperty.MP_NORMAL)
  ml.recompile_material(m);ea.save_loaded_asset(m,False)
 note('Materials ready')
 for g in d['groups']:
  name=g['name'];options=u.FbxImportUI()
  for k,v in dict(import_mesh=True,import_materials=False,import_textures=False,import_as_skeletal=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH,automated_import_should_detect_type=False).items():options.set_editor_property(k,v)
  for k,v in dict(combine_meshes=True,auto_generate_collision=False,generate_lightmap_u_vs=False,convert_scene=False,convert_scene_unit=False,normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS).items():options.static_mesh_import_data.set_editor_property(k,v)
  existing=u.load_asset(BASE+'/Meshes/SM_'+name)
  if existing:existing.set_editor_property('static_materials',[])
  task=u.AssetImportTask()
  for k,v in dict(filename=str(S/'Meshes'/(name+'.obj')),destination_path=BASE+'/Meshes',destination_name='SM_'+name,replace_existing=True,automated=True,save=True,factory=u.FbxFactory(),options=options).items():task.set_editor_property(k,v)
  at.import_asset_tasks([task]);me=u.load_asset(BASE+'/Meshes/SM_'+name);assert me,name
  first=next(line.split()[1] for line in (S/'Meshes'/(name+'.obj')).open() if line.startswith('usemtl '));assigned=set()
  for i,slot in enumerate(me.static_materials):
   key=str(slot.material_slot_name);key=first if key=='defaultMat' else key;assert key in g['materials'],(name,key)
   mat=u.load_asset(g['materials'][key]);assert mat;me.set_material(i,mat);assigned.add(key)
  assert assigned==set(g['materials']),(name,assigned)
  ea.save_loaded_asset(me,False);note('Imported '+name+' / '+str(g['triangles'])+' triangles')
 ea.save_directory(BASE,False,True)
 (P/'Saved'/(REPORT+'.json')).write_text(json.dumps(dict(status='passed',meshes=len(d['groups']),characters=len(d['characters'])),indent=2))
 note('SUCCESS: cast imported; existing levels and saves untouched')
except Exception:
 note(traceback.format_exc());raise
