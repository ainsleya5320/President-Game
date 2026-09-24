import unreal as u,json
from pathlib import Path
P=Path(u.Paths.project_dir());D=json.loads((P/'SourceAssets/manifest.json').read_text());B='/Game/OvalOffice';ml=u.MaterialEditingLibrary;ea=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools();report=[]
for info in D['materials']:
 if not info['texture']:continue
 texture=u.load_asset(B+'/Textures/T_'+Path(info['texture']).stem)
 inst=u.load_asset(B+'/Materials/MI_'+info['id'])
 report.append({'name':info['id'],'texture':str(texture),'old_parameter':str(ml.get_material_instance_texture_parameter_value(inst,'Albedo'))})
 name='M_Image_'+info['id'];m=u.load_asset(B+'/Materials/'+name) or at.create_asset(name,B+'/Materials',u.Material,u.MaterialFactoryNew())
 ml.delete_all_material_expressions(m);m.set_editor_property('two_sided',True)
 t=ml.create_material_expression(m,u.MaterialExpressionTextureSample);t.set_editor_property('texture',texture)
 assert ml.connect_material_property(t,'RGB',u.MaterialProperty.MP_BASE_COLOR)
 rough=ml.create_material_expression(m,u.MaterialExpressionConstant);rough.set_editor_property('r',info['roughness']);assert ml.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
 ml.recompile_material(m);ea.save_loaded_asset(m,False);ml.set_material_instance_parent(inst,m);ml.update_material_instance(inst);ea.save_loaded_asset(inst,False)
(P/'Saved/material_repair.json').write_text(json.dumps(report,indent=2))
u.log('OVAL_MATERIAL_REPAIR_COMPLETE')
