import unreal as u,json
from pathlib import Path
P=Path(u.Paths.project_dir());L=u.get_editor_subsystem(u.LevelEditorSubsystem);A=u.get_editor_subsystem(u.EditorAssetSubsystem);E=u.get_editor_subsystem(u.EditorActorSubsystem);ml=u.MaterialEditingLibrary
L.load_level('/Game/AirForceOne/Maps/AirForceOne')
sky=u.load_asset('/Game/AirForceOne/Materials/M_OutsideSky');ml.delete_all_material_expressions(sky)
c=ml.create_material_expression(sky,u.MaterialExpressionConstant3Vector);c.set_editor_property('constant',u.LinearColor(20,28,40,1));ml.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR);ml.recompile_material(sky);A.save_loaded_asset(sky,False)
for a in E.get_all_level_actors():
 if isinstance(a,u.PostProcessVolume) and a.get_actor_label()=='AF1_Interior exposure':
  s=a.get_editor_property('settings');s.set_editor_property('auto_exposure_bias',-4.5);a.set_editor_property('settings',s)
L.save_current_level();u.log('AF1 lighting polish saved')
