import bpy,json,re,math
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parents[1];S=P/'SourceAssets'
def safe(s):return re.sub(r'[^A-Za-z0-9_]+','_',s).strip('_')[:70]
materials={};manifest=[];groups={}
for i,m in enumerate(bpy.data.materials):
 p=m.node_tree.nodes.get('Principled BSDF') if m.use_nodes else None
 tex=next((n.image for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image),None) if m.use_nodes else None
 name='M%02d_%s'%(i,safe(m.name));texture=None
 if tex:
  texture=safe(Path(tex.name).stem)+'.png';tex.filepath_raw=str(S/'Textures'/texture);tex.file_format='PNG';tex.save()
 materials[m.name]={'id':name,'name':m.name,'color':list(m.diffuse_color)[:3],'roughness':float(p.inputs['Roughness'].default_value) if p else .6,'metallic':float(p.inputs['Metallic'].default_value) if p else 0,'texture':texture,'glass':'window glass' in m.name}
def group(o):
 n=o.name.lower();c=' '.join(x.name for x in o.users_collection)
 if '07 |' in c or '08 |' in c:return None
 if '01 |' in c:
  if 'ceiling' in n:return 'Architecture_Ceiling'
  if 'cornice' in n:return 'Architecture_Cornice'
  if 'window glazing' in n:return 'Windows_Glass'
  if any(s in n for s in ['drape','valance','pelmet','curtain','fringe']):return 'Windows_Curtains'
  if any(s in n for s in ['sash','window','architrave']):return 'Windows_Joinery'
  if 'parquet' in n or 'subfloor' in n:return 'Floor_Parquet'
  if 'plaster wall' in n or 'papered stripe' in n:
   center=sum((o.matrix_world@Vector(v) for v in o.bound_box),Vector())/8
   return 'Walls_Back' if center.y>=.3 else 'Walls_Front'
  return 'Architecture_Panelling'
 if '02 |' in c:
  if 'rug' in n or 'carpet' in n:return 'Floor_Rug'
  if any(s in n for s in ['sofa','cushion','pillow','seam','welting']):return 'Sofa_West' if o.location.x<0 else 'Sofa_East'
  return 'Coffee_Table_and_Accessories'
 if '03 |' in c:
  if any(s in n for s in ['chair','cane weave']):return 'Chair_President' if 'executive' in n or 'five star' in n or 'caster' in n else ('Chair_West' if o.location.x<0 else 'Chair_East')
  if any(s in n for s in ['telephone','stationery','blotter','writing','pen','tray']):return 'Desk_Accessories'
  return 'Resolute_Desk'
 if '04 |' in c:
  if any(s in n for s in ['picture frame','canvas']):return 'Painting_West' if o.location.x<0 else 'Painting_East'
  if any(s in n for s in ['bronco','horse','rider','sculpture']):return 'Bronze_Sculpture'
  if 'credenza' in n or 'photograph' in n or 'picture stand' in n:return 'Rear_Credenza_and_Frames'
  return 'Chest_West' if o.location.x<0 else 'Chest_East'
 if '05 |' in c:return 'Flag_United_States' if o.location.x<0 else 'Flag_Presidential'
 if '06 |' in c:
  if any(s in n for s in ['door']):return 'Door_West' if o.location.x<0 else 'Door_East'
  if any(s in n for s in ['fire','mantel','marble','hearth','grate']):return 'Fireplace'
  return 'Lamp_and_Table_West' if o.location.x<0 else 'Lamp_and_Table_East'
 return None
for o in bpy.context.scene.objects:
 if o.hide_render or o.type not in {'MESH','CURVE'}:continue
 key=group(o)
 if key:groups.setdefault(key,[]).append(o)
deps=bpy.context.evaluated_depsgraph_get()
for name,objects in groups.items():
 path=S/'Meshes'/(name+'.obj');offset=1;count=0;used=set()
 with path.open('w',encoding='utf-8') as f:
  f.write('# Authored Blender geometry. Unreal coordinates in centimetres.\no '+name+'\n')
  for o in objects:
   eo=o.evaluated_get(deps);me=eo.to_mesh();me.calc_loop_triangles();wm=o.matrix_world;nm=wm.to_3x3().inverted().transposed();uv=me.uv_layers.active
   last=None
   for tri in me.loop_triangles:
    idx=tri.material_index;m=me.materials[idx] if idx<len(me.materials) else None;mid=materials[m.name]['id'] if m else list(materials.values())[0]['id'];used.add(mid)
    if mid!=last:f.write('usemtl '+mid+'\n');last=mid
    # Blender (+Y forward, +X right) -> Unreal (+X forward, +Y right).
    for li in reversed(tri.loops):
     loop=me.loops[li];v=wm@me.vertices[loop.vertex_index].co;n=(nm@me.corner_normals[li].vector).normalized();t=uv.data[li].uv if uv else (0,0)
     f.write('v %.5f %.5f %.5f\n'%(v.y*100,v.x*100,v.z*100));f.write('vt %.6f %.6f\n'%(t[0],t[1]));f.write('vn %.6f %.6f %.6f\n'%(n.y,n.x,n.z))
    f.write('f %d/%d/%d %d/%d/%d %d/%d/%d\n'%tuple(v for i in range(offset,offset+3) for v in (i,i,i)));offset+=3;count+=1
   eo.to_mesh_clear()
 manifest.append({'name':name,'triangles':count,'objects':len(objects),'materials':sorted(used)})
 print('EXPORTED',name,count,flush=True)
(S/'manifest.json').write_text(json.dumps({'groups':manifest,'materials':list(materials.values())},indent=2))
print('EXPORT_COMPLETE',len(groups),sum(g['triangles'] for g in manifest),flush=True)
