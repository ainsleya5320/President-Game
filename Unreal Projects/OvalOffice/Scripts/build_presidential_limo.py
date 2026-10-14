"""Blender-authored presidential cabin inspired by the user's reference photo.
Metres in Blender, centimetres in Unreal, +X along the cabin. No add-ons needed.
"""
import bpy,math,json,shutil,sys
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parents[1];S=P/'SourceAssets/PresidentialLimo'
for folder in ['Meshes','Textures','Renders']:(S/folder).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
metadata=[]
def material(name,color,rough=.5,metal=0,texture=None,normal=None,emission=0):
 m=bpy.data.materials.new('LM_'+name);m.use_nodes=True;b=m.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(*color,1);b.inputs['Roughness'].default_value=rough;b.inputs['Metallic'].default_value=metal
 texname=normalname=None
 for src,isnormal in [(texture,False),(normal,True)]:
  if not src:continue
  dest=S/'Textures'/Path(src).name;shutil.copy2(P/src,dest);im=bpy.data.images.load(str(dest),check_existing=True)
  n=m.node_tree.nodes.new('ShaderNodeTexImage');n.image=im
  if isnormal:
   normalname=dest.name;im.colorspace_settings.name='Non-Color';nm=m.node_tree.nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.3;m.node_tree.links.new(n.outputs['Color'],nm.inputs['Color']);m.node_tree.links.new(nm.outputs['Normal'],b.inputs['Normal'])
  else:
   texname=dest.name;mix=m.node_tree.nodes.new('ShaderNodeMixRGB');mix.blend_type='MULTIPLY';mix.inputs[0].default_value=1;mix.inputs[2].default_value=(*color,1);m.node_tree.links.new(n.outputs['Color'],mix.inputs[1]);m.node_tree.links.new(mix.outputs[0],b.inputs['Base Color'])
 if emission:
  b.inputs['Emission Color'].default_value=(*color,1);b.inputs['Emission Strength'].default_value=emission
  if texture:m.node_tree.links.new(mix.outputs[0],b.inputs['Emission Color'])
 metadata.append(dict(id=m.name,color=color,roughness=rough,metallic=metal,texture=texname,normal=normalname,normal_strength=.08,masked=False,skin=False,two_sided=True,emission=emission))
 return m
leather=material('Ivory_Leather',(.66,.60,.48),.56,normal='SourceAssets/AirForceOne/Textures/cognac_leather_normal.png')
headliner=material('Suede_Headliner',(.55,.52,.44),.93)
wood=material('Walnut_Veneer',(.52,.40,.28),.26,texture='SourceAssets/AirForceOne/Textures/walnut.png',normal='SourceAssets/AirForceOne/Textures/walnut_normal.png')
darkwood=material('Walnut_Edge',(.055,.025,.013),.29)
chrome=material('Brushed_Champagne_Metal',(.55,.50,.39),.25,.82)
black=material('Soft_Black_Trim',(.012,.017,.022),.47)
carpet=material('Wool_Carpet',(.26,.25,.22),.96,normal='SourceAssets/AirForceOne/Textures/hall_carpet_normal.png')
stitch=material('Ivory_Stitching',(.42,.37,.29),.8)
screen=material('Display_Glass',(.025,.075,.11),.24,emission=.6)
led=material('Warm_Cove_Light',(1,.84,.61),.3,emission=5)
glass=material('Cut_Crystal',(.39,.48,.51),.13)
glass.node_tree.nodes.get('Principled BSDF').inputs['Transmission Weight'].default_value=.9
metadata[-1]['opacity']=.28
paper=material('Briefing_Paper',(.77,.75,.65),.85)
seal=material('Presidential_Seal',(1,1,1),.6,texture='SourceAssets/AirForceOne/Textures/presidential_seal.png')
garden=material('Window_View',(.65,.72,.72),.8,texture='Prototype/assets/rose-garden.png',emission=.5)
metadata[-1]['mip']=3

def finish(o,name,mat):
 o.name=name;o.data.materials.append(mat);return o
def box(name,loc,size,mat,bevel=.012):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  b=o.modifiers.new('Crafted radiused edges','BEVEL');b.width=bevel;b.segments=4;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
 return finish(o,name,mat)
def cyl(name,loc,r,depth,mat,rotation=None,segments=40):
 bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=depth,location=loc);o=bpy.context.object
 if rotation:o.rotation_euler=rotation
 b=o.modifiers.new('Machined rim','BEVEL');b.width=.003;b.segments=2;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL');return finish(o,name,mat)
def line(name,points,r,mat):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=2;s=c.splines.new('POLY');s.points.add(len(points)-1)
 for p,v in zip(s.points,points):p.co=(*v,1)
 o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);return finish(o,name,mat)
def panel(name,center,w,h,mat,axis='x'):
 x,y,z=center
 vv=[(x,y-w/2,z-h/2),(x,y+w/2,z-h/2),(x,y+w/2,z+h/2),(x,y-w/2,z+h/2)] if axis=='x' else [(x-w/2,y,z-h/2),(x+w/2,y,z-h/2),(x+w/2,y,z+h/2),(x-w/2,y,z+h/2)]
 me=bpy.data.meshes.new(name);me.from_pydata(vv,[],[(0,1,2,3)]);uv=me.uv_layers.new()
 for p,t in zip(uv.data,[(0,0),(1,0),(1,1),(0,1)]):p.uv=t
 o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o);return finish(o,name,mat)
def label(body,loc,size,mat,rot):
 c=bpy.data.curves.new(body,'FONT');c.body=body;c.align_x='CENTER';c.size=size;c.extrude=.0003;o=bpy.data.objects.new(body,c);bpy.context.collection.objects.link(o);o.location=loc;o.rotation_euler=rot;return finish(o,body,mat)

# Softly curved shell, true window openings and substantial door pillars.
box('Acoustic floor',(0,0,-.065),(5.45,2.64,.13),black)
box('Wool fitted carpet',(-.05,0,.008),(4.95,2.42,.025),carpet,.025)
box('Bound centre runner',(-.2,0,.026),(3.65,.85,.014),headliner,.04)
for side in [-1,1]:
 y=side*1.29
 box('Armoured lower door',(0,y,.46),(5.4,.12,.92),black,.04)
 box('Bookmatched walnut lower fascia',(0,side*1.215,.67),(5.20,.06,.50),wood,.025)
 box('Chrome waist reveal',(0,side*1.175,.94),(5.18,.025,.018),chrome,.008)
 box('Padded window sill',(0,side*1.16,.99),(5.16,.15,.085),leather,.035)
 for x in [-2.57,-1.31,.25,1.75,2.45]:
  box('Upholstered door pillar',(x,side*1.22,1.43),(.16,.16,.94),leather,.06)
 box('Rear quarter upholstered panel',(2.13,side*1.24,1.45),(.70,.16,.94),leather,.055)
 for x,w in [(-1.94,1.08),(-.53,1.35),(1.0,1.30)]:
  box('Deep window surround',(x,side*1.26,1.44),(w+.08,.07,.79),black,.07)
  panel('Soft garden beyond tinted glass',(x,side*1.205,1.44),w,.69,garden,'y')
 for x in [-1.9,1.05]:
  box('Recessed door handle surround',(x,side*1.148,.87),(.25,.026,.09),darkwood,.026)
  box('Satin door release',(x,side*1.125,.883),(.16,.035,.025),chrome,.009)
 # Continuous concealed lighting follows the roof shoulder.
 line('Warm roof cove',[(-2.5,side*1.14,1.85),(2.45,side*1.14,1.85)],.012,led)
 line('Padded roof edge',[(-2.55,side*1.19,1.88),(2.48,side*1.19,1.88)],.038,leather)

vv=[];ff=[]
for row,x in enumerate([-2.7,-1.8,-.9,0,.9,1.8,2.55]):
 for j in range(33):
  y=-1.32+j*2.64/32;z=2.11-.27*(abs(y)/1.32)**2;vv.append((x,y,z))
for i in range(6):
 for j in range(32):a=i*33+j;ff.append((a,a+1,a+34,a+33))
me=bpy.data.meshes.new('Continuous curved headliner');me.from_pydata(vv,[],ff);o=bpy.data.objects.new(me.name,me);bpy.context.collection.objects.link(o);finish(o,me.name,headliner)
for p in me.polygons:p.use_smooth=True
for x in [-1.8,-.25,1.3]:
 line('Headliner stitched seam',[(x,-1.25+j*2.5/32,2.107-.27*(abs(-1.25+j*2.5/32)/1.32)**2) for j in range(33)],.002,stitch)
 box('Ceiling light console',(x,0,2.055),(.32,.27,.045),leather,.07)
 for y in [-.075,.075]:
  cyl('Reading lamp chrome bezel',(x,y,2.025),.046,.015,chrome)
  cyl('Reading lamp lens',(x,y,2.013),.033,.014,led)
 for side in [-1,1]:
  cyl('Circular air vent rim',(x,side*.83,1.98),.058,.025,chrome)
  cyl('Air vent insert',(x,side*.83,1.964),.046,.02,black)
  for d in [-.025,0,.025]:line('Vent louver',[(x-.035,side*.83+d,1.95),(x+.035,side*.83+d,1.95)],.003,chrome)

# Presidential rear bench: broad contoured cushions, bolsters, piping and belts.
box('Rear bulkhead',(2.55,0,1.07),(.13,2.6,2.18),leather,.09)
box('Bench walnut plinth',(2.13,0,.24),(.74,2.24,.45),darkwood,.07)
box('Bench cream seat base',(2.10,0,.43),(.73,2.28,.20),leather,.09)
for y in [-.69,0,.69]:
 box('Sculpted seat cushion',(2.03,y,.505),(.65,.665,.13),leather,.06)
 back=box('Contoured seat back',(2.37,y,.90),(.18,.665,.70),leather,.085);back.rotation_euler.y=-.10
 box('Lumbar cushion',(2.26,y,.78),(.12,.53,.28),leather,.055)
 box('Shoulder bolster',(2.29,y,1.14),(.12,.56,.18),leather,.06)
 box('Adjustable head restraint',(2.36,y,1.37),(.22,.47,.30),leather,.09)
 for dy in [-.245,.245]:line('Seat back double stitching',[(2.253,y+dy,.68),(2.20,y+dy,1.20)],.0025,stitch)
 line('Cushion front piping',[(1.711,y-.28,.51),(1.699,y,.515),(1.711,y+.28,.51)],.0035,stitch)
 belt=box('Seat belt webbing',(2.03,y+.25,.58),(.35,.032,.012),black,.004);belt.rotation_euler.z=.12
 box('Seat belt buckle',(1.89,y+.22,.595),(.065,.045,.025),chrome,.008)
 box('Buckle release',(1.875,y+.22,.61),(.035,.027,.012),darkwood,.003)
for y in [-1.10,1.10]:box('Rear padded armrest',(2.02,y,.76),(.72,.17,.20),leather,.06)

# A low return bench on the right leaves the centre aisle clear.
box('Side seat plinth',(-.45,.99,.26),(2.50,.49,.40),darkwood,.05)
for x in [-1.25,-.45,.35]:
 box('Side seat cushion',(x,.97,.49),(.77,.48,.14),leather,.06)
 box('Side seat back',(x,1.16,.80),(.77,.13,.55),leather,.06)
 line('Side seat stitch',[(x-.30,.735,.50),(x+.30,.735,.50)],.0025,stitch)
for x in [-1.72,.82]:box('Side bench arm',(x,.96,.66),(.14,.53,.23),leather,.055)

# Walnut hospitality cabinet: open shelf, fold-down tray and fitted glassware.
box('Hospitality cabinet carcass',(-.35,-1.01,.57),(2.12,.40,1.04),darkwood,.055)
box('Cabinet walnut fascia',(-.35,-.785,.54),(2.02,.045,.88),wood,.02)
for x in [-1.17,.46]:
 box('Inset cabinet door',(x,-.75,.48),(.34,.045,.67),wood,.018)
 box('Cabinet satin handle',(x+.11,-.714,.63),(.025,.035,.16),chrome,.008)
box('Black recessed cabinet niche',(-.34,-.739,.62),(1.11,.015,.45),black,.02)
box('Fold-down service tray',(-.34,-.55,.52),(1.10,.47,.035),wood,.016)
for x in [-.87,.19]:line('Tray retaining rail',[(x,-.77,.55),(x,-.33,.55)],.008,chrome)
line('Tray front rail',[(-.87,-.32,.55),(.19,-.32,.55)],.008,chrome)
box('Cabinet illuminated shelf',(-.34,-.73,.86),(1.02,.20,.025),wood)
line('Cabinet concealed lamp',[(-.82,-.73,.88),(.14,-.73,.88)],.006,led)
for x in [-.66,-.37,-.08]:
 cyl('Crystal tumbler base',(x,-.49,.56),.047,.025,glass)
 verts=[];faces=[]
 for r,z in [(.044,.57),(.044,.67),(.040,.67),(.040,.575)]:
  for j in range(32):a=j*math.tau/32;verts.append((x+r*math.cos(a),-.49+r*math.sin(a),z))
 for ring in range(3):
  for j in range(32):a=ring*32+j;b=ring*32+(j+1)%32;faces.append((a,b,b+32,a+32))
 me=bpy.data.meshes.new('Hollow crystal tumbler');me.from_pydata(verts,[],faces);o=bpy.data.objects.new(me.name,me);bpy.context.collection.objects.link(o);finish(o,me.name,glass)
 for j in range(12):
  a=j*math.tau/12;line('Cut crystal highlight',[(x+.043*math.cos(a),-.49+.043*math.sin(a),.575),(x+.043*math.cos(a),-.49+.043*math.sin(a),.66)],.0015,chrome)
for x in [-.67,-.39,-.11]:
 cyl('Water bottle',(x,-.91,.70),.035,.22,glass)
 cyl('Bottle cap',(x,-.91,.825),.022,.03,chrome)

# Front privacy partition, screens and secure communications.
box('Front privacy bulkhead',(-2.65,0,1.02),(.13,2.60,2.05),leather,.075)
box('Front walnut console',(-2.47,0,.76),(.28,2.10,.50),wood,.055)
box('Privacy screen surround',(-2.555,0,1.42),(.045,1.20,.65),black,.045)
panel('Privacy display',(-2.525,0,1.42),1.10,.55,screen)
label('SECURE MOTORCADE',(-2.51,0,1.47),.055,paper,(math.pi/2,0,math.pi/2))
label('WASHINGTON  /  EN ROUTE',(-2.50,0,1.29),.031,paper,(math.pi/2,0,math.pi/2))
for y in [-.70,.70]:
 cyl('Partition speaker grille',(-2.555,y,1.55),.083,.015,black,(0,math.pi/2,0))
 for j in range(7):line('Speaker grille line',[(-2.54,y-.055,1.51+j*.012),(-2.54,y+.055,1.51+j*.012)],.0015,chrome)
box('Rear seal walnut plaque',(2.455,0,1.85),(.045,.42,.23),wood,.06)
panel('Rear presidential seal',(2.422,0,1.85),.19,.19,seal)
box('Cabinet television frame',(-1.92,-1.115,1.48),(.52,.085,.35),black,.035)
panel('Cabinet television',(-1.92,-1.067,1.48),.46,.285,screen,'y')
label('LIVE  /  MOTORCADE',(-1.92,-1.055,1.48),.025,paper,(math.pi/2,0,0))

box('Communications side console',(1.17,-.99,.66),(.70,.43,.95),wood,.05)
box('Secure phone cradle',(1.19,-.94,1.16),(.25,.20,.045),black,.025)
box('Correctly oriented phone handset',(1.19,-.94,1.21),(.28,.065,.055),black,.025)
for x in [-.11,.11]:box('Handset earpiece',(1.19+x,-.94,1.19),(.065,.085,.075),black,.025)
for i in range(3):
 for j in range(4):box('Phone keypad',(1.12+i*.045,-1.025+j*.026,1.19),(.026,.016,.008),chrome,.003)
line('Coiled phone cord',[(1.32+.014*math.cos(t*.8),-.97+.014*math.sin(t*.8),1.16-t*.002) for t in range(65)],.003,black)
box('Presidential briefing folio',(-2.28,.48,1.025),(.25,.33,.024),darkwood,.008)
box('Briefing pages',(-2.28,.48,1.041),(.22,.29,.006),paper,.002)
for side in [-1,1]:
 for x in [-2.1,.95]:
  cyl('Lower door speaker',(x,side*1.153,.29),.093,.016,black,(math.pi/2,0,0))
  cyl('Courtesy lamp bezel',(x+.22,side*1.145,.25),.035,.018,chrome,(math.pi/2,0,0))
  cyl('Courtesy lamp',(x+.22,side*1.13,.25),.023,.014,led,(math.pi/2,0,0))

scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True;scene.world=bpy.data.worlds.new('Exterior daylight');scene.world.color=(.08,.09,.11);scene.view_settings.view_transform='AgX'
for loc,target,power,size in [((-.5,0,1.98),(0,0,.3),160,2.5),((1.3,0,1.94),(2.2,0,.7),100,1.4),((-.5,-1.0,1.55),(1,.4,.8),180,2.4),((-1.8,.95,1.6),(.8,0,1),180,2)]:
 d=bpy.data.lights.new('Soft cabin illumination','AREA');d.energy=power;d.shape='DISK';d.size=size;o=bpy.data.objects.new(d.name,d);scene.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
d=bpy.data.cameras.new('Passenger view');cam=bpy.data.objects.new(d.name,d);scene.collection.objects.link(cam);cam.location=(-1.9,.40,1.36);cam.rotation_euler=(Vector((1.35,-.35,1.05))-cam.location).to_track_quat('-Z','Y').to_euler();d.lens=20;scene.camera=cam
scene.render.resolution_x=1500;scene.render.resolution_y=1000;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.filepath=str(S/'Renders/Limo_Interior.png')
scene.view_settings.exposure=-.65
for im in bpy.data.images:
 if im.source=='FILE':im.filepath='//Textures/'+Path(im.filepath).name
bpy.ops.wm.save_as_mainfile(filepath=str(S/'Presidential_Limo.blend'),compress=True)

deps=bpy.context.evaluated_depsgraph_get();count=0;offset=1;used={}
with (S/'Meshes/Interior.obj').open('w') as f:
 f.write('mtllib Staff.mtl\no Interior\n')
 for o in scene.objects:
  if o.type not in ['MESH','CURVE','FONT']:continue
  eo=o.evaluated_get(deps);me=eo.to_mesh();me.calc_loop_triangles();uv=me.uv_layers.active;nm=o.matrix_world.to_3x3().inverted().transposed();last=None
  for tri in me.loop_triangles:
   key=me.materials[tri.material_index].name;used[key]='/Game/PresidentialLimo/Materials/'+key
   if key!=last:f.write('usemtl '+key+'\n');last=key
   for li in tri.loops:
    v=o.matrix_world@me.vertices[me.loops[li].vertex_index].co;n=(nm@me.corner_normals[li].vector).normalized();t=uv.data[li].uv if uv else (0,0)
    if not uv:
     axis=max(range(3),key=lambda i:abs((nm@tri.normal)[i]));axes=[i for i in range(3) if i!=axis];t=(v[axes[0]],v[axes[1]])
    # Legacy Unreal OBJ import reflects Y. Counter-reflect positions, normals
    # and winding so the finished cabin matches the native interaction layout.
    f.write('v %.5f %.5f %.5f\nvt %.6f %.6f\nvn %.6f %.6f %.6f\n'%(v.x*100,-v.y*100,v.z*100,t[0],t[1],n.x,-n.y,n.z))
   f.write('f '+' '.join('%d/%d/%d'%(i,i,i) for i in reversed(range(offset,offset+3)))+'\n');offset+=3;count+=1
  eo.to_mesh_clear()
(S/'Meshes/Staff.mtl').write_text(''.join('newmtl '+m['id']+'\nKd 1 1 1\n' for m in metadata))
(S/'manifest.json').write_text(json.dumps(dict(materials=metadata,groups=[dict(name='Interior',triangles=count,materials=used)],characters=[]),indent=2))
print('LIMO EXPORTED',count,flush=True)
if '--export-only' not in sys.argv:bpy.ops.render.render(write_still=True)
