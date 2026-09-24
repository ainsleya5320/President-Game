import bpy, math, json, gzip
from pathlib import Path
P=Path(__file__).parent
scene=bpy.context.scene
# Preserve the source scene; this process writes only new exports.
for o in bpy.data.objects:
 o.select_set(False)
for o in list(scene.objects):
 collection=' '.join(c.name for c in o.users_collection)
 if o.hide_render or o.type in {'LIGHT','CAMERA'} or '07 |' in collection:
  bpy.data.objects.remove(o,do_unlink=True)
for o in list(scene.objects):
 if o.type not in {'MESH','CURVE'}:continue
 o.hide_set(False);o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(P/'Oval_Office.glb'),export_format='GLB',use_selection=True,export_apply=True,export_cameras=False,export_lights=False)
print('FULL_EXPORT_READY',flush=True)
# Lightweight spatial preview of the same actual Blender geometry.
# Color materials retain the palette; portable full export keeps picture textures.
for im in bpy.data.images:
 if im.source=='FILE' and im.size[0]>0:
  width,height=im.size;scale=min(1,256/max(width,height));im.scale(max(1,int(width*scale)),max(1,int(height*scale)))
# Remove micro-detail invisible at room scale and allow a south-facing cutaway.
small_names=['Beaded','fringe','cane weave','Telephone keypad','telephone cord','Cushion piping','Apple stem','Garden','Photographic','Sash crossbar']
for o in list(scene.objects):
 if any(n.lower() in o.name.lower() for n in small_names):
  bpy.data.objects.remove(o,do_unlink=True);continue
 for mod in list(o.modifiers):
  if mod.type=='BEVEL':mod.segments=1
  elif mod.type=='WEIGHTED_NORMAL':o.modifiers.remove(mod)
 if o.type=='CURVE':o.data.bevel_resolution=0;o.data.resolution_u=1
 if o.type=='MESH' and len(o.data.polygons)>500:
  mod=o.modifiers.new('Interactive simplification','DECIMATE');mod.ratio=min(1,400/len(o.data.polygons))
# Group architectural pieces for interactive visibility.
architecture=bpy.data.collections.get('01 | Elliptical architecture')
for o in list(scene.objects):
 if architecture in o.users_collection:
  if o.name.startswith('Ceiling'):o.name='ROOF__'+o.name
  elif any(k in o.name for k in ['cornice','Cornice']):o.name='UPPER__'+o.name
  elif any(k in o.name for k in ['plaster wall','papered stripe','wainscot','Wainscot','Raised panel','dado moulding']):
   # Wall objects store world-coordinate vertices; use their real bounding center.
   from mathutils import Vector
   center=sum((o.matrix_world@Vector(v) for v in o.bound_box),Vector())/8
   o.name=('FRONTWALL__' if center.y<.5 else 'BACKWALL__')+o.name
scene.world=None
for o in scene.objects:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(P/'Oval_Office_Preview.glb'),export_format='GLB',use_selection=True,export_apply=True,export_cameras=False,export_lights=False)
data=(P/'Oval_Office_Preview.glb').read_bytes();packed=gzip.compress(data,compresslevel=9)
(P/'Oval_Office_Preview.glb.gz').write_bytes(packed)
print('PREVIEW_BYTES',len(data),'GZIP_BYTES',len(packed),flush=True)
