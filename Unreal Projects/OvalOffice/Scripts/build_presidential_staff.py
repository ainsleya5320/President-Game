"""Build the game's clothed human cast in Blender from CC0 MakeHuman source assets.

Run Blender --background --python Scripts/build_presidential_staff.py.
Source files and provenance live in SourceAssets/PresidentialStaff. Generated OBJ
meshes use centimetres, +X forward, +Z up, matching the existing Unreal actors.
No add-on is installed and no downloaded Python code is executed.
"""
import bpy, json, math, gzip, shutil, sys
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

P=Path(__file__).resolve().parents[1]
S=P/'SourceAssets/PresidentialStaff'; V=S/'Vendor'; A=V/'assets'
for d in ['Meshes','Textures','Renders']: (S/d).mkdir(parents=True,exist_ok=True)
MATERIALS={}; EXPORTS=[]; CHARACTERS=[]

# These are fictional characters. Ancestry values only select an artistic base mesh.
CAST=[
 dict(id='Chen',name='Maya Chen',sex='female',ancestry='asian',age=.25,height=1.67,hair='ponytail01',suit=(.24,.32,.44),hair_tint=(.32,.25,.21)),
 dict(id='Brooks',name='Daniel Brooks',sex='male',ancestry='caucasian',age=.40,height=1.81,hair='short02',suit=(.42,.44,.48),hair_tint=(.53,.47,.39)),
 dict(id='Morgan',name='Agent Morgan',sex='male',ancestry='african',age=.20,height=1.85,hair='short01',suit=(.20,.23,.29),hair_tint=(.25,.22,.20)),
 dict(id='Ruiz',name='Elena Ruiz',sex='female',ancestry='caucasian',age=.24,height=1.70,hair='braid01',suit=(.44,.23,.25),hair_tint=(.50,.31,.18)),
 dict(id='Bell',name='Samira Bell',sex='female',ancestry='african',age=.32,height=1.72,hair='afro01',suit=(.20,.36,.36),hair_tint=(.28,.22,.18)),
 dict(id='Mercer',name='Evelyn Mercer',sex='female',ancestry='caucasian',age=.67,height=1.66,hair='short02',suit=(.43,.39,.32),hair_tint=(.80,.77,.71)),
 dict(id='Ortega',name='Luis Ortega',sex='male',ancestry='caucasian',age=.55,height=1.77,hair='short02',suit=(.23,.29,.36),hair_tint=(.36,.32,.28)),
 dict(id='Ellis',name='Jordan Ellis',sex='male',ancestry='asian',age=.08,height=1.76,hair='short01',suit=(.37,.32,.27),hair_tint=(.30,.24,.20)),
]

def objdata(path):
 verts=[];uvs=[];faces=[];faceuv=[];groups={};group='body'
 for line in path.read_text(errors='replace').splitlines():
  s=line.split()
  if not s:continue
  if s[0]=='v':verts.append(Vector(tuple(map(float,s[1:4]))))
  elif s[0]=='vt':uvs.append(tuple(map(float,s[1:3])))
  elif s[0]=='g':group=s[1]
  elif s[0]=='f':
   f=[int(t.split('/')[0])-1 for t in s[1:]];faces.append(f)
   faceuv.append([int(t.split('/')[1])-1 if '/' in t and t.split('/')[1] else 0 for t in s[1:]])
   groups.setdefault(group,[]).append(len(faces)-1)
 return dict(v=verts,uv=uvs,f=faces,fu=faceuv,g=groups)

BASE=objdata(V/'base.obj')
RIG=json.loads((V/'rig.game_engine.json').read_text())
WEIGHTS=json.loads((V/'weights.game_engine.json').read_text())['weights']
BASE_WEIGHTS=[{} for _ in BASE['v']]
for bone,rows in WEIGHTS.items():
 for i,w in rows:BASE_WEIGHTS[i][bone]=w

def to_blender(v):return Vector((v.x*.1,-v.z*.1,v.y*.1))

def shape(c):
 verts=[v.copy() for v in BASE['v']]
 for age,factor in [('young',1-c['age']),('old',c['age'])]:
  with gzip.open(V/(c['ancestry']+'-'+c['sex']+'-'+age+'.target.gz'),'rt') as f:
   for line in f:
    t=line.split()
    if len(t)==4 and not line.startswith('#'):verts[int(t[0])]+=Vector(tuple(map(float,t[1:])))*factor
 return verts

def material(name,file=None,color=(1,1,1),rough=.65,masked=False,normal=None,skin=False):
 if name in MATERIALS and name in bpy.data.materials:return bpy.data.materials[name]
 m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes;l=m.node_tree.links
 bs=n.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough
 bs.inputs['Specular IOR Level'].default_value=.28
 if skin:bs.inputs['Subsurface Weight'].default_value=.07
 out_file=None;out_normal=None
 if file:
  out_file=Path(file).name;dest=S/'Textures'/out_file
  if not dest.exists():shutil.copy2(file,dest)
  tex=n.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(dest),check_existing=True)
  if color!=(1,1,1):
   mix=n.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1;mix.inputs[2].default_value=(*color,1);l.new(tex.outputs['Color'],mix.inputs[1]);l.new(mix.outputs[0],bs.inputs['Base Color'])
  else:l.new(tex.outputs['Color'],bs.inputs['Base Color'])
  if masked:l.new(tex.outputs['Alpha'],bs.inputs['Alpha']);m.surface_render_method='DITHERED'
 if normal and Path(normal).exists():
  out_normal=Path(normal).name;dest=S/'Textures'/out_normal
  if not dest.exists():shutil.copy2(normal,dest)
  tex=n.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(dest),check_existing=True);tex.image.colorspace_settings.name='Non-Color'
  nm=n.new('ShaderNodeNormalMap');l.new(tex.outputs['Color'],nm.inputs['Color']);l.new(nm.outputs['Normal'],bs.inputs['Normal'])
 MATERIALS[name]=dict(id=name,texture=out_file,normal=out_normal,color=color,roughness=rough,masked=masked,skin=skin)
 return m

def mesh(name,data,verts,face_ids,mat,weights=None):
 used=sorted({i for fi in face_ids for i in data['f'][fi]});remap={v:i for i,v in enumerate(used)}
 me=bpy.data.meshes.new(name);me.from_pydata([verts[i] for i in used],[],[[remap[i] for i in data['f'][f]] for f in face_ids]);me.update()
 o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o);me.materials.append(mat)
 uv=me.uv_layers.new()
 for poly,fi in zip(me.polygons,face_ids):
  poly.use_smooth=True
  for li,ti in zip(poly.loop_indices,data['fu'][fi]):uv.data[li].uv=data['uv'][ti] if data['uv'] else (0,0)
 if weights:
  groups={}
  for idx,old in enumerate(used):
   total=sum(weights[old].values())
   for bone,w in weights[old].items():
    if w<.0001:continue
    if bone not in groups:groups[bone]=o.vertex_groups.new(name=bone)
    groups[bone].add([idx],w/max(total,.0001),'REPLACE')
 return o

def fit(path,verts):
 lines=path.read_text().splitlines();settings={};rows=[];delete=set();mode='header'
 for line in lines:
  t=line.split()
  if not t or t[0].startswith('#'):continue
  if t[0]=='verts':mode='verts';continue
  if t[0]=='delete_verts':mode='delete';continue
  if mode=='header':settings[t[0]]=t[1:]
  elif mode=='verts':
   if t[0].isdigit():rows.append(t)
   else:settings[t[0]]=t[1:]
  elif mode=='delete':
   i=0
   while i<len(t):
    if i+2<len(t) and t[i+1]=='-':delete.update(range(int(t[i]),int(t[i+2])+1));i+=3
    elif t[i].isdigit():delete.add(int(t[i]));i+=1
    else:i+=1
 scales=[]
 for axis,dim in [('x_scale',0),('y_scale',1),('z_scale',2)]:
  p=settings.get(axis);scales.append(abs(verts[int(p[0])][dim]-verts[int(p[1])][dim])/float(p[2]) if p else 1)
 coords=[];weights=[]
 for row in rows:
  if len(row)==1:
   i=int(row[0]);coords.append(verts[i].copy());weights.append(BASE_WEIGHTS[i].copy());continue
  ids=list(map(int,row[:3]));ws=list(map(float,row[3:6]));off=Vector(tuple(float(x)*s for x,s in zip(row[6:9],scales)))
  coords.append(sum((verts[i]*w for i,w in zip(ids,ws)),Vector())+off)
  weight={}
  for i,w in zip(ids,ws):
   for b,bw in BASE_WEIGHTS[i].items():weight[b]=weight.get(b,0)+w*bw
  weights.append(weight)
 return settings,coords,weights,delete

def bonepoint(info,verts):
 if info['strategy']=='CUBE':
  ids={i for f in BASE['g'][info['cube_name']] for i in BASE['f'][f]}
 elif info['strategy']=='MEAN':ids=info['vertex_indices']
 else:return Vector(info['default_position'])
 return sum((to_blender(verts[i]) for i in ids),Vector())/len(ids)

def rig(verts,name):
 data=bpy.data.armatures.new(name);o=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(o)
 bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
 for name,info in RIG.items():
  b=data.edit_bones.new(name);b.head=bonepoint(info['head'],verts);b.tail=bonepoint(info['tail'],verts);b.roll=info['roll']
 for name,info in RIG.items():
  if info['parent']:data.edit_bones[name].parent=data.edit_bones[info['parent']]
 bpy.ops.object.mode_set(mode='OBJECT');o.select_set(False);return o

def aim(rig,name,direction):
 p=rig.pose.bones[name];m=p.matrix.copy();rot=m.to_quaternion();q=(rot@Vector((0,1,0))).rotation_difference(Vector(direction).normalized())
 p.matrix=Matrix.Translation(m.translation)@(q@rot).to_matrix().to_4x4();bpy.context.view_layer.update()

def pose(rig,seated=False):
 for b in rig.pose.bones:b.matrix_basis=Matrix.Identity(4)
 bpy.context.view_layer.update()
 for side,sign in [('l',1),('r',-1)]:
  aim(rig,'upperarm_'+side,(sign*.105,.006,-.34))
  aim(rig,'lowerarm_'+side,(sign*.025,-.22 if seated else -.045,-.12 if seated else -.28))
  # Relaxed hands, not the splayed default modelling pose.
  for prefix in ['index','middle','ring','pinky']:
   for j in [1,2,3]:
    name=f'{prefix}_0{j}_{side}'
    if name in rig.pose.bones:
     b=rig.pose.bones[name];b.rotation_mode='XYZ';b.rotation_euler.x=math.radians(12 if j==1 else 18)
  if seated:
   aim(rig,'thigh_'+side,(sign*.025,-.43,-.015));aim(rig,'calf_'+side,(sign*.005,.025,-.43));aim(rig,'foot_'+side,(0,-.17,-.055))
  else:
   aim(rig,'thigh_'+side,(sign*.028,0,-.44));aim(rig,'calf_'+side,(sign*.009,.006,-.43))
 bpy.context.view_layer.update()

def evaluated(o):
 deps=bpy.context.evaluated_depsgraph_get();eo=o.evaluated_get(deps);me=bpy.data.meshes.new_from_object(eo,depsgraph=deps)
 n=bpy.data.objects.new(o.name+'_baked',me);bpy.context.collection.objects.link(n);return n

def export(name,objects,origin=Vector()):
 path=S/'Meshes'/(name+'.obj');count=0;offset=1;used={};deps=bpy.context.evaluated_depsgraph_get()
 with path.open('w') as f:
  f.write('mtllib Staff.mtl\no '+name+'\n')
  for o in objects:
   eo=o.evaluated_get(deps);me=eo.to_mesh();me.calc_loop_triangles();uv=me.uv_layers.active;last=None
   for tri in me.loop_triangles:
    mat=me.materials[tri.material_index];key=mat.name;used[key]='/Game/PresidentialStaff/Materials/'+key
    if key!=last:f.write('usemtl '+key+'\n');last=key
    for li in tri.loops:
     v=o.matrix_world@me.vertices[me.loops[li].vertex_index].co-origin;n=me.corner_normals[li].vector;t=uv.data[li].uv if uv else (0,0)
     f.write('v %.5f %.5f %.5f\nvt %.6f %.6f\nvn %.6f %.6f %.6f\n'%(-v.y*100,v.x*100,v.z*100,t[0],t[1],-n.y,n.x,n.z))
    f.write('f '+' '.join('%d/%d/%d'%(i,i,i) for i in range(offset,offset+3))+'\n');offset+=3;count+=1
   eo.to_mesh_clear()
 EXPORTS.append(dict(name=name,materials=used,triangles=count));print('STAFF EXPORTED',name,count,flush=True)

def build(c):
 for obj in list(bpy.data.objects):bpy.data.objects.remove(obj,do_unlink=True)
 bpy.data.orphans_purge(do_recursive=True)
 raw=shape(c);verts=[to_blender(v) for v in raw]
 ground=min(v.z for v in verts[:13380]);top=max(v.z for v in verts[:13380]);scale=c['height']/(top-ground)
 skinpath=A/'skins'/('middleage_'+c['ancestry']+'_'+c['sex']);skinfile=next(skinpath.glob('*diffuse*.png'))
 skin=material('ST_Skin_'+c['ancestry']+'_'+c['sex'],skinfile,rough=.58,skin=True)
 r=rig(raw,c['id']+'_Rig');objects=[];deleted=set()
 clothing='male_elegantsuit01' # fitted to each character's own body, including women's pantsuits
 assets=[('clothes',clothing,'suit'),('clothes','shoes03','shoe'),('hair',c['hair'],'hair'),('eyes','low-poly','eye'),('eyebrows','eyebrow001' if c['sex']=='male' else 'eyebrow003','brow')]
 for kind,asset,part in assets:
  path=A/kind/asset;settings,coords,weights,hide=fit(path/(asset+'.mhclo'),raw);deleted.update(hide)
  data=objdata(path/settings['obj_file'][0]);assert len(data['v'])==len(coords),(asset,len(data['v']),len(coords))
  tex=next(path.glob('*diffuse.png'),None);nm=next(path.glob('*normal.png'),None)
  if part=='eye':tex=A/'eyes/materials/brown_eye.png'
  if part=='brow':tex=path/(asset+'.png')
  mat=material('ST_'+c['id']+'_'+part if part in ['suit','hair'] else 'ST_'+asset,tex,c['suit'] if part=='suit' else c['hair_tint'] if part=='hair' else (1,1,1),rough=.84 if part=='suit' else .67 if part=='hair' else .30 if part=='eye' else .42,masked=part in ['hair','brow'],normal=nm)
  o=mesh(c['id']+'_'+part,data,[to_blender(v) for v in coords],range(len(data['f'])),mat,weights);o['part']=part
  mod=o.modifiers.new('Human pose','ARMATURE');mod.object=r;objects.append(o)
 # Drop helper geometry and skin hidden by garments. Keep head and hands with UVs intact.
 faces=[i for i in BASE['g']['body'] if not all(v in deleted for v in BASE['f'][i])]
 body=mesh(c['id']+'_skin',BASE,verts,faces,skin,BASE_WEIGHTS);body['part']='skin';mod=body.modifiers.new('Human pose','ARMATURE');mod.object=r;objects.append(body)
 # Head / body split below the collar. The runtime pivots the complete textured head.
 neck=bonepoint(RIG['neck_01']['head'],raw);cut=neck.z-.04
 pose(r,False);bpy.context.view_layer.update()
 head=[];standing=[];seated=[]
 for is_seated,dest in [(False,standing),(True,seated)]:
  pose(r,is_seated)
  for o in objects:
   n=evaluated(o)
   for v in n.data.vertices:v.co=(v.co-Vector((0,0,ground)))*scale
   if o['part'] in ['hair','eye','brow']:
    if not is_seated:head.append(n)
    else:bpy.data.objects.remove(n,do_unlink=True)
    continue
   if o['part']=='skin':
    # Keep exactly the same seam for both poses; head is rigid above the lower neck.
    import bmesh
    if not is_seated:
     hn=n.copy();hn.data=n.data.copy();bpy.context.collection.objects.link(hn)
     bm=bmesh.new();bm.from_mesh(hn.data)
     bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.00001,plane_co=(0,0,(cut-ground)*scale),plane_no=(0,0,1),clear_inner=True)
     bm.to_mesh(hn.data);bm.free();head.append(hn)
    bm=bmesh.new();bm.from_mesh(n.data)
    bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.00001,plane_co=(0,0,(cut-ground)*scale),plane_no=(0,0,1),clear_outer=True)
    bm.to_mesh(n.data);bm.free()
   # Moderate reduction preserves face/hand detail; the suit keeps its fitted silhouette.
   if o['part']=='suit':
    dec=n.modifiers.new('Game silhouette budget','DECIMATE');dec.ratio=.72
   dest.append(n)
 # Place seated pelvis 54 cm above the ground; compensate using actual rig landmark.
 hip=(bonepoint(RIG['pelvis']['head'],raw).z-ground)*scale
 seatdrop=hip-.54
 for o in seated:
  for v in o.data.vertices:v.co.z-=seatdrop
 # Different leg lengths share the existing chair height. Plant each shoe sole
 # without moving hips off the cushion or changing the NPC's collision footprint.
 for parts,hinge in [(standing,.12),(seated,.48)]:
  low=min(v.co.z for o in parts for v in o.data.vertices)
  for o in parts:
   for v in o.data.vertices:
    if v.co.z<hinge:v.co.z=(v.co.z-low)*hinge/(hinge-low)
 headpivot=(neck-Vector((0,0,ground)))*scale
 export(c['id']+'_Head',head,headpivot);export(c['id']+'_Standing',standing);export(c['id']+'_Seated',seated)
 CHARACTERS.append(dict(id=c['id'],name=c['name'],height=c['height']*100,headStanding=[-headpivot.y*100,headpivot.x*100,headpivot.z*100],headSeated=[-headpivot.y*100,headpivot.x*100,(headpivot.z-seatdrop)*100]))
 # Preserve an editable, posed Blender source with the rig, original skinned parts and baked game meshes.
 for o in objects:o.hide_render=True;o.hide_set(True)
 r.hide_render=True;r.hide_set(True)
 for o in seated:o.hide_render=True;o.hide_set(True)
 for img in bpy.data.images:
  if img.source=='FILE' and Path(img.filepath).suffix.lower() in ['.png','.jpg','.jpeg']:img.filepath='//Textures/'+Path(img.filepath).name
 bpy.ops.wm.save_as_mainfile(filepath=str(S/(c['id']+'.blend')),compress=True)
 render(c,standing+head)

def render(c,objects):
 scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
 scene.render.resolution_x=760;scene.render.resolution_y=960;scene.render.resolution_percentage=100
 scene.world.color=(.12,.12,.12);scene.view_settings.view_transform='AgX';scene.view_settings.exposure=-.7
 floor=material('ST_Studio',color=(.08,.105,.13),rough=.8)
 bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.006));bpy.context.object.data.materials.append(floor)
 def light(loc,power,size):
  d=bpy.data.lights.new('Softbox','AREA');d.energy=power;d.shape='DISK';d.size=size;o=bpy.data.objects.new('Softbox',d);scene.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,1.2))-o.location).to_track_quat('-Z','Y').to_euler()
 light((2,-3,3),420,3);light((-2,-1,2),220,2.5);light((1,2,3),500,2)
 d=bpy.data.cameras.new('Portrait');cam=bpy.data.objects.new('Portrait',d);scene.collection.objects.link(cam);scene.camera=cam;d.lens=58
 cam.location=(.7,-3.50,1.48);cam.rotation_euler=(Vector((0,0,.93))-cam.location).to_track_quat('-Z','Y').to_euler()
 scene.render.filepath=str(S/'Renders'/(c['id']+'.png'));bpy.ops.render.render(write_still=True)
 # Close portrait used for an actual facial-detail review, not a substitute for the in-game view.
 scene.render.resolution_x=720;scene.render.resolution_y=720;cam.location=(.30,-1.12,c['height']-.10);d.lens=66
 cam.rotation_euler=(Vector((0,-.01,c['height']-.18))-cam.location).to_track_quat('-Z','Y').to_euler()
 scene.render.filepath=str(S/'Renders'/(c['id']+'_Face.png'));bpy.ops.render.render(write_still=True)

chosen=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
for c in CAST:
 if not chosen or c['id'] in chosen:build(c)
(S/'manifest.json').write_text(json.dumps(dict(materials=list(MATERIALS.values()),groups=EXPORTS,characters=CHARACTERS),indent=2))
if not chosen:
 catalog=P/'Source/OvalOffice/Private/FourYears/PresidentialStaffCatalog.inl'
 catalog.write_text('// Generated by build_presidential_staff.py. Centimetres, +X forward.\n'+ '\n'.join('{TEXT("%s"), FVector(%s), FVector(%s)},'%(c['id'],','.join('%.5ff'%v for v in c['headStanding']),','.join('%.5ff'%v for v in c['headSeated'])) for c in CHARACTERS)+'\n')
with (S/'Meshes/Staff.mtl').open('w') as f:
 for m in MATERIALS.values():f.write('newmtl '+m['id']+'\nKd 1 1 1\n')
print('STAFF BUILD COMPLETE',flush=True)
