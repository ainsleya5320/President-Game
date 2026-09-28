"""Revise existing scenes without regenerating their architecture or furnishings.
Usage: blender -b --python refresh_decor_blender.py -- --original-root <President-Game>
After the first run, the revised packed .blend sources are self contained.
"""
import bpy,math,json,re,shutil,sys,argparse
from pathlib import Path
from mathutils import Vector,Matrix
P=Path(__file__).resolve().parents[1]; S=P/'SourceAssets/DecorRefresh'
for d in ['Textures','Meshes','Renders']:(S/d).mkdir(parents=True,exist_ok=True)
args=argparse.ArgumentParser();args.add_argument('--original-root',type=Path)
args=args.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
art=json.loads((S/'art_provenance.json').read_text()); meta={};exports=[];placements=[]
def safe(s):return re.sub('[^A-Za-z0-9_]+','_',s).strip('_')
def material(name,color=(1,1,1),rough=.65,metal=0,file=None):
 name='DR_'+name;m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True
 m.node_tree.nodes.clear();n=m.node_tree.nodes;b=n.new('ShaderNodeBsdfPrincipled');out=n.new('ShaderNodeOutputMaterial');m.node_tree.links.new(b.outputs[0],out.inputs[0])
 b.inputs['Base Color'].default_value=(*color,1);b.inputs['Roughness'].default_value=rough;b.inputs['Metallic'].default_value=metal
 if file:
  t=n.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(S/'Textures'/file),check_existing=True);m.node_tree.links.new(t.outputs['Color'],b.inputs['Base Color'])
 meta[name]=dict(id=name,color=color,roughness=rough,metallic=metal,texture=file)
 return m
def artmat(index):return material('Art_'+str(art[index]['id']),file=art[index]['file'])
def finish(o,name,mat,group):
 o.name=name;o['decor_group']=group
 if mat:o.data.materials.append(mat)
 return o
def box(name,loc,size,mat,group,bevel=.006):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  b=o.modifiers.new('Rounded manufactured edges','BEVEL');b.width=bevel;b.segments=3;o.modifiers.new('Face weighted normals','WEIGHTED_NORMAL')
 return finish(o,name,mat,group)
def mesh(name,verts,faces,mat,group,uv=None,smooth=False):
 me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);o=bpy.data.objects.new(name,me);bpy.context.scene.collection.objects.link(o);finish(o,name,mat,group)
 if uv:
  me.uv_layers.new()
  for poly in me.polygons:
   poly.use_smooth=smooth
   for li in poly.loop_indices:me.uv_layers.active.data[li].uv=uv[me.loops[li].vertex_index]
 return o
def cord(name,points,radius,mat,group):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=radius;c.bevel_resolution=3;s=c.splines.new('POLY');s.points.add(len(points)-1)
 for p,co in zip(s.points,points):p.co=(*co,1)
 o=bpy.data.objects.new(name,c);bpy.context.scene.collection.objects.link(o);return finish(o,name,mat,group)
def label(body,loc,size,mat,group,rot=(0,0,0)):
 c=bpy.data.curves.new('Telephone '+body,'FONT');c.body=body;c.align_x='CENTER';c.size=size;c.extrude=.00005
 o=bpy.data.objects.new(c.name,c);bpy.context.scene.collection.objects.link(o);o.location=loc;o.rotation_euler=rot;return finish(o,o.name,mat,group)
def phone(center,angle,group):
 # Local front = -Y, all controls readable from the seated user's side.
 before=set(bpy.data.objects);black=material('Phone_Graphite',(.023,.029,.033),.39);rubber=material('Phone_Rubber',(.012,.013,.016),.8)
 buttons=material('Phone_Buttons',(.11,.13,.14),.47);white=material('Phone_Legends',(.77,.79,.73),.6);screen=material('Phone_LCD',(.11,.22,.23),.33)
 w=.285;d=.255
 verts=[(x,y,z) for z in [0,1] for y in [-d/2,d/2] for x in [-w/2,w/2]]
 verts=[(x,y,.012 if z==0 else .045+(y+d/2)*.25) for x,y,z in verts]
 o=mesh('Telephone inclined console',verts,[(0,2,3,1),(4,5,7,6),(0,1,5,4),(1,3,7,5),(3,2,6,7),(2,0,4,6)],black,group)
 b=o.modifiers.new('Console edge radius','BEVEL');b.width=.012;b.segments=4;o.modifiers.new('Console normals','WEIGHTED_NORMAL')
 slope=math.atan(.25)
 def top(y):return .045+(y+d/2)*.25
 def key(name,x,y,w,h,mat=buttons):
  o=box(name,(x,y,top(y)+.005),(w,h,.008),mat,group,.003);o.rotation_euler.x=slope;return o
 key('Telephone display bezel',.037,.071,.150,.083,black)
 glass=key('Telephone LCD glass',.037,.071,.125,.057,screen);glass.location.z+=.004
 label('SECURE LINE',(.037,.078,top(.078)+.016),.008,white,group,(slope,0,0))
 label('READY',(.037,.055,top(.055)+.016),.007,white,group,(slope,0,0))
 for row in range(4):
  for col in range(3):
   x=-.008+col*.040;y=.005-row*.030;key('Telephone numbered key',x,y,.032,.022)
   txt=str(row*3+col+1) if row<3 else ['*','0','#'][col]
   label(txt,(x,y-.003,top(y)+.010),.010,white,group,(slope,0,0))
 for y in [-.035,.0,.035]:key('Telephone line selector',.114,y,.017,.022)
 # One connected receiver with distinct rounded earpiece and mouthpiece.
 for y in [-.091,.091]:
  key('Telephone cradle',-.105,y,.063,.053,black)
  o=box('Telephone receiver end',(-.105,y,top(y)+.033),(.065,.057,.040),black,group,.017);o.rotation_euler.x=slope
 o=box('Telephone receiver grip',(-.105,0,top(0)+.040),(.034,.16,.025),black,group,.011);o.rotation_euler.x=slope
 for i in range(7):key('Telephone speaker slit',.121,.07+i*.005,.011,.0015,rubber)
 points=[(-.132,-.087,.077),(-.172,-.105,.043)]
 points += [(-.179+.008*math.cos(i*math.tau/12),-.10+i*.00055,.030+.008*math.sin(i*math.tau/12)) for i in range(220)]
 points += [(-.17,.03,.029),(-.14,.08,.04)]
 cord('Telephone coiled receiver lead',points,.0019,rubber,group)
 rot=Matrix.Rotation(angle,4,'Z')
 for o in set(bpy.data.objects)-before:o.matrix_world=Matrix.Translation(Vector(center))@rot@o.matrix_world
 placements.append(dict(scene=SCENE,kind='telephone',center=center,front=[math.sin(angle),-math.cos(angle),0]))
def draped_flag(origin,angle,mat,group,top=3.30):
 # Still-air, gathered display: the fly falls down alongside the vertical hoist.
 # Long vertical flutes and a low free corner replace the former horizontal wave.
 gold=material('Flag_Bullion',(.61,.39,.105),.64,.22);x,y=origin;nx=90;ny=52
 def point(u,v):
  # Monotonic spread avoids self-overlapping knife folds; depth supplies the
  # rounded flutes of soft indoor cloth, rather than an accordion silhouette.
  spread=.49*math.sin(math.pi*u/2)
  fold=.073*math.sin(u*math.tau*3.25+.25)*math.sin(math.pi*u)**.40
  xx=.025+spread
  yy=fold-.038*u+.022*v
  zz=top-1.42*v-1.28*u+.20*u*v-.018*math.sin(u*math.tau*5.25)*v
  return (x+xx*math.cos(angle)-yy*math.sin(angle),y+xx*math.sin(angle)+yy*math.cos(angle),zz)
 verts=[point(i/nx,j/ny) for j in range(ny+1) for i in range(nx+1)];faces=[]
 for j in range(ny):
  for i in range(nx):k=j*(nx+1)+i;faces.append((k,k+1,k+nx+2,k+nx+1))
 uv=[(i/nx,1-j/ny) for j in range(ny+1) for i in range(nx+1)]
 o=mesh('Gravity draped indoor standard',verts,faces,mat,group,uv,True)
 sol=o.modifiers.new('Silk thickness','SOLIDIFY');sol.thickness=.0012
 # Woven bullion binding plus individually hanging fringe threads on free edges.
 for edge in [[point(i/150,1) for i in range(151)],[point(1,j/110) for j in range(111)]]:cord('Flag sewn bullion edge',edge,.006,gold,group)
 for i in range(145):
  p=Vector(point(i/144,1));cord('Individual flag fringe',[p,p+Vector((.001,.001,-.037))],.0014,gold,group)
 for i in range(105):
  p=Vector(point(1,i/104));cord('Fly edge fringe',[p,p+Vector((.005,0,-.035))],.0014,gold,group)
 for z in [top-.02,top-1.40]:cord('Flag staff attachment',[(x,y,z),point(0,(top-z)/1.42)],.004,gold,group)
 placements.append(dict(scene=SCENE,kind='indoor_flag',pole=origin,top=top,fly_width=.55,lowest=min(v[2] for v in verts)))
def picture(name,center,normal,w,h,mat,group):
 n=Vector(normal).normalized();up=Vector((0,0,1));right=up.cross(n);c=Vector(center)
 gold=material('Gilded_Frame',(.58,.37,.11),.31,.67);dark=material('Frame_Recess',(.045,.025,.012),.7)
 def rail(x,z,ww,hh,depth,offset,ma):
  o=box(name+' frame',c+right*x+up*z+n*offset,(ww,depth,hh),ma,group,.009);o.rotation_euler.z=math.atan2(right.y,right.x)
 rail(0,0,w+.15,h+.15,.04,0,dark)
 for q in [-1,1]:
  rail(q*(w/2+.039),0,.07,h+.15,.060,.027,gold);rail(0,q*(h/2+.039),w+.08,.07,.060,.027,gold)
 verts=[c+right*x*w/2+up*y*h/2+n*.061 for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]]
 o=mesh(name,verts,[(0,1,2,3)],mat,group,[(0,0),(1,0),(1,1),(0,1)])
 fit_uv(o,w/h,mat)
def fit_uv(o,ratio,mat):
 image=next((n.image for n in mat.node_tree.nodes if n.type=='TEX_IMAGE'),None)
 if not image or not o.data.uv_layers.active:return
 aspect=image.size[0]/image.size[1]
 for li,t in enumerate(o.data.uv_layers.active.data):
  # These artwork cards are quads; reset before proportional cropping on reruns.
  u,v=[(0,0),(1,0),(1,1),(0,1)][o.data.loops[li].vertex_index%4]
  if ratio<aspect:u=.5+(u-.5)*ratio/aspect
  else:v=.5+(v-.5)*aspect/ratio
  t.uv=(u,v)
def delete(objs):
 for o in list(objs):bpy.data.objects.remove(o,do_unlink=True)
def export(scene,groups,old):
 mapping={m.get('name',m['id']):dict(id=m['id'],asset='/Game/'+('OvalOffice' if scene=='Oval' else scene)+'/Materials/'+('M_' if scene=='AirForceOne' else 'MI_')+m['id']) for m in old['materials']}
 mapping.update({name:dict(id=name,asset='/Game/DecorRefresh/Materials/'+name) for name in meta})
 deps=bpy.context.evaluated_depsgraph_get()
 for group,objects in groups.items():
  name=scene+'_'+group;path=S/'Meshes'/(name+'.obj');offset=1;used={};tris=0
  with path.open('w') as f:
   f.write('mtllib Decor.mtl\no '+name+'\n')
   for o in objects:
    eo=o.evaluated_get(deps);me=eo.to_mesh();me.calc_loop_triangles();wm=o.matrix_world;nm=wm.to_3x3().inverted().transposed();uv=me.uv_layers.active;last=None
    for tri in me.loop_triangles:
     mat=me.materials[tri.material_index];info=mapping[mat.name];mid=info['id'];used[mid]=info['asset']
     if mid!=last:f.write('usemtl '+mid+'\n');last=mid
     native_phone=scene=='Oval' and o.name.startswith('Telephone')
     for li in reversed(tri.loops) if scene=='Oval' and not native_phone else tri.loops:
      v=wm@me.vertices[me.loops[li].vertex_index].co;n=(nm@me.corner_normals[li].vector).normalized();t=uv.data[li].uv if uv else (0,0)
      if scene=='Oval':
       # The legacy Oval exporter reflects the room's visual handedness in UE.
       # Preserve the phone's position but make its legends/left receiver readable.
       if native_phone:v.x=-1.36-v.x;n.x=-n.x
       v=Vector((v.y,v.x,v.z));n=Vector((n.y,n.x,n.z))
      elif scene=='AirForceOne':v=Vector((v.y,-v.x,v.z));n=Vector((n.y,-n.x,n.z))
      f.write('v %.5f %.5f %.5f\nvt %.6f %.6f\nvn %.6f %.6f %.6f\n'%(v.x*100,v.y*100,v.z*100,t[0],t[1],n.x,n.y,n.z))
     f.write('f '+' '.join('%d/%d/%d'%(i,i,i) for i in range(offset,offset+3))+'\n');offset+=3;tris+=1
    eo.to_mesh_clear()
  exports.append(dict(name=name,scene=scene,replace_group=group,materials=used,triangles=tris));print('EXPORTED',name,tris,flush=True)
 bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(S/(scene+'_Decor.blend')))

for SCENE in ['Oval','AirForceOne','CabinetRoom']:
 source=S/(SCENE+'_Decor.blend')
 if args.original_root and SCENE!='CabinetRoom':source=args.original_root/('oval-office/Oval_Office.blend' if SCENE=='Oval' else 'air-force-one/Air_Force_One.blend')
 elif SCENE=='CabinetRoom':source=P/'SourceAssets/CabinetRoom/Cabinet_Room.blend'
 bpy.ops.wm.open_mainfile(filepath=str(source))
 old=json.loads((P/'SourceAssets'/('manifest.json' if SCENE=='Oval' else SCENE+'/manifest.json')).read_text())
 groups={}
 if SCENE=='Oval':
  delete(o for o in bpy.context.scene.objects if o.get('decor_group')=='Fireplace_Paintings' or (o.get('decor_group') in ['Flag_United_States','Flag_Presidential'] and not o.name.startswith(('Flagpole','Finial'))))
  # Preserve the original accessory group, replacing only its telephone components.
  code=(P/'Scripts/export_from_blender.py').read_text();scope={'Vector':Vector};exec(code[code.index('def group(o):'):code.index('for o in bpy.context.scene.objects:')],scope)
  for o in list(bpy.context.scene.objects):
   if o.type not in {'MESH','CURVE','FONT'}:continue
   g=scope['group'](o)
   if g in ['Desk_Accessories','Rear_Credenza_and_Frames','Flag_United_States','Flag_Presidential']:o['decor_group']=g
  delete(o for o in bpy.context.scene.objects if any(s in o.name.lower() for s in ['telephone','coiled telephone']))
  flagobjects=[o for o in bpy.context.scene.objects if any('05 |' in c.name for c in o.users_collection)]
  flagmats=[bpy.data.materials.get('United States flag | woven silk'),next(m for m in bpy.data.materials if 'presidential' in m.name.lower() and ('flag' in m.name.lower() or 'standard' in m.name.lower()))]
  keep=[o for o in flagobjects if o.name.startswith(('Flagpole','Finial'))]
  delete(o for o in flagobjects if o not in keep)
  for o in keep:
   o['decor_group']='Flag_United_States' if o.location.x<0 else 'Flag_Presidential'
  for x,ma,g in zip([-1.27,1.27],flagmats,['Flag_United_States','Flag_Presidential']):draped_flag((x,4.2),0 if x<0 else math.pi,ma,g,3.38)
  phone((-.68,2.71,.923),math.pi,'Desk_Accessories')
  photos=['family-beach.png','family-garden.png','family-dog.png','coastal-memory.png','volunteer-day.png','art_121377.jpg','art_71971.jpg','art_152747.jpg']
  for i,o in enumerate(sorted([o for o in bpy.context.scene.objects if o.name.startswith('Framed desk photograph')],key=lambda o:o.name)):
   m=material('Memory_'+str(i),file=photos[i]);o.data.materials.clear();o.data.materials.append(m);fit_uv(o,.16/(.15+(i%3)*.02),m)
  picture('Washington over the Oval mantel',(0,-5.12,2.72),(0,1,0),.78,1.10,material('Oval_Washington',file='washington_portrait.png'),'Fireplace_Paintings')
  picture('Oval mountain landscape',(-2.24,-4.40,2.39),(.45,1,0),1.05,.72,artmat(1),'Fireplace_Paintings')
  picture('Oval Niagara landscape',(2.24,-4.40,2.39),(-.45,1,0),1.05,.72,artmat(2),'Fireplace_Paintings')
 elif SCENE=='AirForceOne':
  for o in bpy.context.scene.objects:
   key=safe(o.users_collection[0].name) if o.users_collection else ''
   if key in ['06_Desk_objects_and_cabin_details','10_Presidential_seals_and_gallery']:o['decor_group']=key
  delete(o for o in bpy.context.scene.objects if any(s in o.name.lower() for s in ['telephone','phone inclined','handset']))
  phone((-1.77,1.27,.80),math.atan2(.60,.79),'06_Desk_objects_and_cabin_details')
  phone((.23,9.02,.812),math.pi/2,'06_Desk_objects_and_cabin_details')
  indices=[0,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
  for o,idx in zip(sorted([o for o in bpy.context.scene.objects if o.name.endswith('| artwork')],key=lambda o:o.name),indices):
   m=artmat(idx);o.data.materials.clear();o.data.materials.append(m)
   points=[o.matrix_world@v.co for v in o.data.vertices];w=(points[1]-points[0]).length;h=(points[2]-points[1]).length;fit_uv(o,w/h,m)
   placements.append(dict(scene=SCENE,kind='painting',object=o.name,title=art[idx]['title']))
 else:
  for o in bpy.context.scene.objects:
   if o.get('export_group')=='Fireplace_and_Art':o['decor_group']='Fireplace_and_Art'
  delete(o for o in bpy.context.scene.objects if o.name.startswith('Ceremonial flag'))
  for y,ma in [(-2.70,bpy.data.materials['US_Flag']),(2.70,bpy.data.materials['Presidential_Flag'])]:draped_flag((5.87,y),math.pi/2 if y<0 else -math.pi/2,ma,'Fireplace_and_Art',3.01)
  for name,idx in [('Washington above the mantel',22),('American landscape',18),('Liberty portrait',19),('River valley',21)]:
   o=bpy.data.objects[name];m=artmat(idx);o.data.materials.clear();o.data.materials.append(m);fit_uv(o,(o.data.vertices[1].co-o.data.vertices[0].co).length/(o.data.vertices[2].co-o.data.vertices[1].co).length,m)
   placements.append(dict(scene=SCENE,kind='painting',object=name,title=art[idx]['title']))
 for o in bpy.context.scene.objects:
  if o.type in {'MESH','CURVE','FONT'} and o.get('decor_group') and not o.hide_render:groups.setdefault(o['decor_group'],[]).append(o)
 export(SCENE,groups,old)
(S/'manifest.json').write_text(json.dumps(dict(materials=list(meta.values()),groups=exports,placements=placements),indent=2))
allids={mid for g in exports for mid in g['materials']}
(S/'Meshes/Decor.mtl').write_text(''.join('newmtl '+mid+'\nKd 0.7 0.7 0.7\nd 1\n\n' for mid in sorted(allids)).rstrip()+'\n')
print('DECOR_AUTHORING_COMPLETE',flush=True)
