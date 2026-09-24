import bpy,json
from pathlib import Path
P=Path(__file__).parent
s=bpy.context.scene
missing=[]
for im in bpy.data.images:
 if im.source=='FILE' and not im.packed_file and not Path(bpy.path.abspath(im.filepath)).exists():missing.append(im.filepath)
assert not missing,missing
s.camera=bpy.data.objects['01 | Oval Office architectural view']
s.render.resolution_x=1800;s.render.resolution_y=1238;s.render.resolution_percentage=100
s.cycles.samples=128;s.cycles.use_denoising=True
s.render.filepath='//renders/Oval_Office_Hero.png'
for m in bpy.data.materials:
 if m.name.startswith('Resolute |'):
  p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Coat Weight'].default_value=.22;p.inputs['Coat Roughness'].default_value=.19
bpy.ops.file.pack_all()
bpy.ops.wm.save_as_mainfile(filepath=str(P/'Oval_Office.blend'))
report={'objects':len(s.objects),'rendered_objects':sum(not o.hide_render for o in s.objects),'materials':len(bpy.data.materials),'cameras':[o.name for o in s.objects if o.type=='CAMERA'],'packed_images':[im.name for im in bpy.data.images if im.packed_file],'missing_external_images':missing,'renderer':s.render.engine,'blender_version':bpy.app.version_string,'render_size':[1800,1238],'samples':128}
(P/'scene_report.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
