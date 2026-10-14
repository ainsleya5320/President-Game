"""Give the Cabinet windows a continuous, softly focused photographic garden.

Run in Unreal Editor, outside PIE. Only the Cabinet map and its own panorama
material are saved. Existing lighting, interiors and gameplay remain intact.
The project's existing rose-garden.png supplies the garden; the Oval Office
environment remains behind it at extreme viewing angles.
"""
import unreal as u
import json
from pathlib import Path

P=Path(u.Paths.project_dir())
ea=u.get_editor_subsystem(u.EditorAssetSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
ml=u.MaterialEditingLibrary
path='/Game/CabinetRoom/Materials/M_PhotographicGarden'
m=u.load_asset(path) or u.AssetToolsHelpers.get_asset_tools().create_asset(
    'M_PhotographicGarden','/Game/CabinetRoom/Materials',u.Material,u.MaterialFactoryNew())
ml.delete_all_material_expressions(m)
m.set_editor_property('two_sided',True)
m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
texture_path='/Game/CabinetRoom/Textures/T_PhotographicRoseGarden'
texture=u.load_asset(texture_path)
if not texture:
    task=u.AssetImportTask()
    for k,v in dict(filename=str(P/'Prototype/assets/rose-garden.png'),destination_path='/Game/CabinetRoom/Textures',destination_name='T_PhotographicRoseGarden',automated=True,save=True,factory=u.TextureFactory()).items():task.set_editor_property(k,v)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=u.load_asset(texture_path)
assert texture
texture.set_editor_property('power_of_two_mode',u.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
texture.set_editor_property('mip_gen_settings',u.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
ea.save_loaded_asset(texture,False)
t=ml.create_material_expression(m,u.MaterialExpressionTextureSampleParameter2D)
t.set_editor_property('parameter_name','Garden')
t.set_editor_property('texture',texture)
# A fixed photo mip softens foliage without blurring people or room detail.
t.set_editor_property('mip_value_mode',u.TextureMipValueMode.TMVM_MIP_LEVEL)
t.set_editor_property('const_mip_value',4)
gain=ml.create_material_expression(m,u.MaterialExpressionMultiply)
gain.set_editor_property('const_b',3.2)
assert ml.connect_material_expressions(t,'RGB',gain,'A')
assert ml.connect_material_property(gain,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
ml.layout_material_expressions(m)
ml.recompile_material(m)
ea.save_loaded_asset(m,False)

levels.load_level('/Game/CabinetRoom/Maps/CabinetRoom')
removed=[];panorama=None
for actor in list(actors.get_all_level_actors()):
    if actor.get_actor_label() in ['CR_Rose_Garden','CR_Photographic Rose Garden']:
        removed.append(actor.get_actor_label());actors.destroy_actor(actor)
    elif actor.get_actor_label()=='CR_Garden panorama':panorama=actor
assert panorama,'Cabinet panorama missing'
panorama.static_mesh_component.set_material(0,u.load_asset('/Game/OvalOffice/Materials/M_GardenPanorama'))
panorama.static_mesh_component.set_cast_shadow(False)
panorama.set_actor_enable_collision(False)
photo=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,2200,-440),u.Rotator(pitch=0,yaw=0,roll=90))
photo.set_actor_label('CR_Photographic Rose Garden');photo.set_folder_path('Cabinet Room')
photo.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'))
photo.static_mesh_component.set_material(0,m);photo.static_mesh_component.set_cast_shadow(False)
photo.set_actor_scale3d(u.Vector(60,33.75,1));photo.set_actor_enable_collision(False)
levels.save_current_level()
(P/'Saved/CabinetGardenValidation.json').write_text(json.dumps(dict(
    status='passed',removed=removed,material=path,photographic_source=texture_path,mip=4),indent=2))
u.log('CABINET GARDEN: photographic environment saved')
