import bpy
from pathlib import Path
P=Path(__file__).parent;s=bpy.context.scene
# Keep the connecting doorways physically clear for the eventual game level.
for o in list(s.objects):
 if o.name.startswith('Hall walnut lower rail') and abs(o.location.x-1.48)<.001:bpy.data.objects.remove(o,do_unlink=True)
 elif o.name.startswith('Continuous hallway handrail') and o.type=='CURVE':
  if abs(o.data.splines[0].points[0].co.x-1.48)<.001:bpy.data.objects.remove(o,do_unlink=True)
c=bpy.data.collections['03 | Passage and sliding doors']
for a,b in [(0,3.05),(4.10,5.55),(6.60,11.6)]:
 bpy.ops.mesh.primitive_cube_add(size=1,location=(1.48,(a+b)/2,.33));o=bpy.context.object;o.name='Hall walnut lower rail';o.dimensions=(.025,b-a,.64);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(bpy.data.materials['Bookmatched walnut veneer'])
 for co in list(o.users_collection):co.objects.unlink(o)
 c.objects.link(o)
 cu=bpy.data.curves.new('Handrail','CURVE');cu.dimensions='3D';cu.bevel_depth=.018;cu.bevel_resolution=2;sp=cu.splines.new('POLY');sp.points.add(1);sp.points[0].co=(1.48,a+.04,.90,1);sp.points[1].co=(1.48,b-.04,.90,1);o=bpy.data.objects.new('Continuous hallway handrail',cu);c.objects.link(o);cu.materials.append(bpy.data.materials['Champagne satin metal'])
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(P/'Air_Force_One.blend'));print('DOORWAYS_CLEAR')
