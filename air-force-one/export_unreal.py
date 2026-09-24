import bpy,json,re
from pathlib import Path
P=Path(__file__).parent
S=P.parent/'Unreal Projects'/'OvalOffice'/'SourceAssets'/'AirForceOne'
for d in ['Meshes','Textures']: (S/d).mkdir(parents=True,exist_ok=True)
def safe(s):return re.sub(r'[^A-Za-z0-9_]+','_',s).strip('_')[:70]
materials={};groups={};manifest=[]
for i,m in enumerate(bpy.data.materials):
 bs=m.node_tree.nodes.get('Principled BSDF') if m.use_nodes else None
 maps={}
 for n in m.node_tree.nodes if m.use_nodes else []:
  if n.type=='TEX_IMAGE' and n.image:
   image=n.image;filename=safe(Path(image.name).stem)+'.png'
   image.filepath_raw=str(S/'Textures'/filename);image.file_format='PNG';image.save()
   maps['normal' if image.colorspace_settings.name=='Non-Color' else 'texture']=filename
 strength=next((float(n.inputs['Strength'].default_value) for n in m.node_tree.nodes if n.type=='NORMAL_MAP'),.2) if m.use_nodes else .2
 materials[m.name]={'id':'AF%02d_%s'%(i,safe(m.name)),'name':m.name,'color':list(bs.inputs['Base Color'].default_value)[:3] if bs else list(m.diffuse_color)[:3],'roughness':float(bs.inputs['Roughness'].default_value) if bs else .6,'metallic':float(bs.inputs['Metallic'].default_value) if bs else 0,'normal_strength':strength,'glass':m.name=='Window glass',**maps}
for o in bpy.context.scene.objects:
 if o.hide_render or o.type not in {'MESH','CURVE','FONT'} or o.name.startswith('EXTERIOR'):continue
 key=safe(o.users_collection[0].name)
 if 'glass' in o.name.lower():key='Window_Glass'
 elif o.name.startswith('CEILING'):key='Ceiling'
 groups.setdefault(key,[]).append(o)
deps=bpy.context.evaluated_depsgraph_get()
for name,objects in groups.items():
 offset=1;count=0;used=set()
 with (S/'Meshes'/(name+'.obj')).open('w',encoding='utf-8') as f:
  f.write('mtllib ../materials.mtl\n# Authored cabin geometry in Unreal centimetres\no '+name+'\n')
  for o in objects:
   eo=o.evaluated_get(deps);me=eo.to_mesh();me.calc_loop_triangles();wm=o.matrix_world;nm=wm.to_3x3().inverted().transposed();uv=me.uv_layers.active;last=None
   for tri in me.loop_triangles:
    m=me.materials[tri.material_index] if tri.material_index<len(me.materials) else None
    mid=materials[m.name]['id'] if m else list(materials.values())[0]['id'];used.add(mid)
    if mid!=last:f.write('usemtl '+mid+'\n');last=mid
    # OBJ importer performs its own handedness conversion (negates Y).
    # This rotation yields native Unreal X=Blender Y, Y=Blender X, Z=Blender Z.
    for li in tri.loops:
     loop=me.loops[li];v=wm@me.vertices[loop.vertex_index].co;n=(nm@me.corner_normals[li].vector).normalized();t=uv.data[li].uv if uv else (0,0)
     f.write('v %.5f %.5f %.5f\n'%(v.y*100,-v.x*100,v.z*100));f.write('vt %.6f %.6f\n'%(t[0],t[1]));f.write('vn %.6f %.6f %.6f\n'%(n.y,-n.x,n.z))
    f.write('f %d/%d/%d %d/%d/%d %d/%d/%d\n'%tuple(v for i in range(offset,offset+3) for v in (i,i,i)));offset+=3;count+=1
   eo.to_mesh_clear()
 manifest.append({'name':name,'triangles':count,'objects':len(objects),'materials':sorted(used)})
 print('EXPORTED',name,count,flush=True)
(S/'manifest.json').write_text(json.dumps({'groups':manifest,'materials':list(materials.values())},indent=2))
(S/'materials.mtl').write_text('\n'.join('newmtl '+m['id']+'\nKd '+' '.join(map(str,m['color']))+'\nKs 0.2 0.2 0.2\nNs 32\nd 1\n' for m in materials.values()))
print('EXPORT_COMPLETE',len(groups),sum(g['triangles'] for g in manifest),flush=True)
