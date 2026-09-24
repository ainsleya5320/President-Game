import bpy,sys
from pathlib import Path
P=Path(__file__).parent
s=bpy.context.scene
mode=sys.argv[-1]
s.render.image_settings.file_format='PNG'
s.cycles.use_denoising=True
if mode=='detail':
 s.camera=bpy.data.objects['02 | Resolute desk and south windows'];s.render.resolution_x=1500;s.render.resolution_y=1100;s.cycles.samples=72
 s.render.filepath=str(P/'renders'/'Oval_Office_Desk.png')
elif mode=='preview':
 s.render.resolution_x=1000;s.render.resolution_y=688;s.cycles.samples=24;s.render.filepath=str(P/'renders'/'preview-refined.png')
else:
 s.camera=bpy.data.objects['01 | Oval Office architectural view'];s.render.resolution_x=1800;s.render.resolution_y=1238;s.cycles.samples=128;s.render.filepath=str(P/'renders'/'Oval_Office_Hero.png')
bpy.ops.render.render(write_still=True)
