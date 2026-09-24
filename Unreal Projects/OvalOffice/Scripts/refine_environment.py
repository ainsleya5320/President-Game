import unreal as u
ml=u.MaterialEditingLibrary;ea=u.EditorAssetLibrary
m=u.load_asset('/Game/OvalOffice/Materials/M_GardenPanorama');ml.delete_all_material_expressions(m)
t=ml.create_material_expression(m,u.MaterialExpressionTextureSampleParameterCube);t.set_editor_property('parameter_name','Garden');t.set_editor_property('texture',u.load_asset('/Game/OvalOffice/Textures/T_garden'));t.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
v=ml.create_material_expression(m,u.MaterialExpressionCameraVectorWS);direction=ml.create_material_expression(m,u.MaterialExpressionMultiply);direction.set_editor_property('const_b',-1)
assert ml.connect_material_expressions(v,'',direction,'A')
assert ml.connect_material_expressions(direction,'',t,'UVs')
gain=ml.create_material_expression(m,u.MaterialExpressionMultiply);gain.set_editor_property('const_b',32)
assert ml.connect_material_expressions(t,'RGB',gain,'A')
assert ml.connect_material_property(gain,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
ml.recompile_material(m);ea.save_loaded_asset(m,False)
