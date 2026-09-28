"""Author the walkable Cabinet Room in Blender; export editable source and UE meshes.
Game adaptation inspired by official White House Cabinet Room tours, not a measured survey.
Blender metres -> Unreal centimetres: X=X, Y=-Y, Z=Z.
"""
import bpy, math, json, random, shutil
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parents[1]; S=P/'SourceAssets'/'CabinetRoom'
for d in ['Meshes','Textures','Renders']: (S/d).mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
random.seed(1970)
GROUP='Architecture'; mats={}; metadata={}

def mat(name,color,rough=.5,metal=0,texture=None):
 m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*color,1)
 b=m.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(*color,1);b.inputs['Roughness'].default_value=rough;b.inputs['Metallic'].default_value=metal
 if texture:
  source=P/'SourceAssets'/texture;dest=S/'Textures'/source.name
  shutil.copy2(source,dest);im=bpy.data.images.load(str(dest),check_existing=True);t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=im
  mult=m.node_tree.nodes.new('ShaderNodeMixRGB');mult.blend_type='MULTIPLY';mult.inputs[0].default_value=1;mult.inputs[2].default_value=(*color,1)
  m.node_tree.links.new(t.outputs['Color'],mult.inputs[1]);m.node_tree.links.new(mult.outputs[0],b.inputs['Base Color'])
 mats[name]=m;metadata[name]={'id':name,'color':color,'roughness':rough,'metallic':metal,'texture':Path(texture).name if texture else None}
 return m
cream=mat('Warm_Limestone',(.66,.61,.50),.76);ivory=mat('Painted_Ivory',(.84,.82,.74),.4)
wood=mat('Mahogany',(.55,.30,.20),.24,texture='AirForceOne/Textures/walnut.png')
edge=mat('Dark_Mahogany',(.20,.075,.033),.27);gold=mat('Antique_Brass',(.57,.36,.12),.27,.74)
leather=mat('Cabinet_Leather',(.018,.025,.027),.39,texture='AirForceOne/Textures/cognac_leather.png')
blue=mat('Federal_Blue_Carpet',(.095,.16,.23),.94);border=mat('Carpet_Border',(.26,.34,.39),.95)
cloth=mat('Ivory_Damask',(.67,.60,.43),.9);paper=mat('Ivory_Paper',(.9,.88,.79),.86)
black=mat('Microphone',(.016,.019,.021),.34);stone=mat('Hearth_Marble',(.64,.61,.52),.22)
green=mat('Garden_Hedges',(.12,.25,.075),.94);grass=mat('Lawn',(.19,.30,.105),1)
rose=mat('White_Roses',(.87,.80,.66),.7);water=mat('Crystal',(.39,.50,.55),.18,.25)
wash=mat('Washington', (1,1,1),.8,texture='AirForceOne/Textures/washington_portrait.png')
land=mat('River_Landscape',(1,1,1),.8,texture='AirForceOne/Textures/river_landscape.png')
liberty=mat('Liberty',(1,1,1),.8,texture='AirForceOne/Textures/statue_liberty.png')
flag=mat('US_Flag',(1,1,1),.83,texture='Textures/us_flag.png')
pflag=mat('Presidential_Flag',(1,1,1),.83,texture='Textures/presidential_flag.png')
seal=mat('Presidential_Seal',(1,1,1),.8,texture='AirForceOne/Textures/presidential_seal.png')

def finish(o,name,m):
 o.name=name;o['export_group']=GROUP
 if m:o.data.materials.append(m)
 return o

def box(name,loc,size,m,bevel=.015):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  b=o.modifiers.new('Soft crafted edges','BEVEL');b.width=bevel;b.segments=3
  o.modifiers.new('Weighted surface normals','WEIGHTED_NORMAL')
 return finish(o,name,m)

def cyl(name,loc,radius,depth,m,vertices=32):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=loc);o=bpy.context.object
 b=o.modifiers.new('Turned edges','BEVEL');b.width=.008;b.segments=2;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
 return finish(o,name,m)

def ball(name,loc,scale,m):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=8,location=loc);o=bpy.context.object;o.scale=scale
 for p in o.data.polygons:p.use_smooth=True
 return finish(o,name,m)

def rod(name,a,b,r,m):
 a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,r,(b-a).length,m,16);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o

def text(name,body,loc,size,m,rot=(math.pi/2,0,0)):
 c=bpy.data.curves.new(name,'FONT');c.body=body;c.align_x='CENTER';c.size=size;c.extrude=.0005;c.space_character=1.15
 o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);o.location=loc;o.rotation_euler=rot;return finish(o,name,m)

def panel_image(name,loc,w,h,m,rot):
 # Local XY plane with image UV, normal local +Z.
 mesh=bpy.data.meshes.new(name);mesh.from_pydata([(-w/2,-h/2,0),(w/2,-h/2,0),(w/2,h/2,0),(-w/2,h/2,0)],[],[(0,1,2,3)]);mesh.uv_layers.new()
 for loop,uv in zip(mesh.uv_layers.active.data,[(0,0),(1,0),(1,1),(0,1)]):loop.uv=uv
 o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o);o.location=loc;o.rotation_euler=rot;return finish(o,name,m)

def frame(name,center,w,h,m,wall='back'):
 # back wall at +Y, or end wall at +X; frame and image face the room.
 x,y,z=center
 if wall=='back':
  box(name+' backing',(x,y+.015,z),(w+.16,.065,h+.16),edge)
  for dx in [-w/2-.035,w/2+.035]:box(name+' gilded stile',(x+dx,y-.01,z),(.065,.07,h+.14),gold,.012)
  for dz in [-h/2-.035,h/2+.035]:box(name+' gilded rail',(x,y-.01,z+dz),(w+.08,.07,.065),gold,.012)
  panel_image(name,(x,y-.053,z),w,h,m,(math.pi/2,0,0))
 else:
  box(name+' backing',(x+.015,y,z),(.065,w+.16,h+.16),edge)
  for dy in [-w/2-.035,w/2+.035]:box(name+' stile',(x-.01,y+dy,z),(.07,.065,h+.14),gold)
  for dz in [-h/2-.035,h/2+.035]:box(name+' rail',(x-.01,y,z+dz),(.07,w+.08,.065),gold)
  panel_image(name,(x-.053,y,z),w,h,m,(math.pi/2,0,-math.pi/2))

# Room shell, raised panels, layered cornice and coffered ceiling.
box('Continuous floor',(0,0,-.055),(13.0,7.6,.11),blue,0)
box('Entrance hall floor',(-7.35,-2.6,-.055),(1.7,2.4,.11),blue,0)
box('West wall south',(-6.55,1.05,1.9),(.18,5.5,3.8),cream)
box('West wall north',(-6.55,-3.65,1.9),(.18,.3,3.8),cream)
box('West door lintel',(-6.55,-2.6,3.4),(.18,1.8,.8),ivory)
box('East fireplace wall',(6.55,0,1.9),(.18,7.6,3.8),cream)
box('Portrait wall',(0,3.85,1.9),(13.2,.18,3.8),cream)
for y in [-3.8,3.8]:
 for z,depth,height in [(.13,.09,.26),(.96,.11,.09),(3.48,.10,.10),(3.60,.17,.12),(3.72,.23,.12)]:box('Long moulding',(0,y,z),(13.0,depth,height),ivory,.014)
for x in [-6.43,6.43]:
 for z,depth,height in [(3.48,.10,.10),(3.60,.17,.12),(3.72,.23,.12)]:box('End crown',(x,0,z),(depth,7.6,height),ivory)
for x in [-5.7,-4.3,-2.9,-1.5,0,1.5,2.9,4.3,5.7]:
 box('Raised wall panel',(x,3.735,.54),(1.27,.06,.66),ivory)
 for dx in [-.55,.55]:box('Panel inset',(x+dx,3.69,.54),(.018,.02,.49),cream,.004)
 for z in [.295,.785]:box('Panel inset',(x,3.69,z),(1.11,.02,.018),cream,.004)
box('Ceiling',(0,0,3.86),(13.2,7.8,.12),ivory)
for x in [-4.4,0,4.4]:box('Ceiling transverse beam',(x,0,3.68),(.22,7.6,.17),ivory)
for y in [-2.4,2.4]:box('Ceiling longitudinal beam',(0,y,3.68),(13,.19,.17),ivory)
# Carpet borders, leaving a blue field beneath the mahogany table.
for y in [-3.35,3.35]:
 box('Carpet broad border',(0,y,.006),(12.1,.16,.008),border,0)
 box('Carpet gold pinstripe',(0,y-.12*math.copysign(1,y),.011),(12.1,.018,.006),gold,0)
for x in [-6.02,6.02]:box('Carpet end border',(x,0,.006),(.16,6.85,.008),border,0)

GROUP='Windows_and_Drapery'
# Five tall garden windows; the wall is actually open here.
for x in [-5.1,-2.55,0,2.55,5.1]:
 box('Window apron',(x,-3.82,.39),(2.15,.16,.78),ivory)
 box('Window head',(x,-3.82,3.45),(2.15,.18,.65),ivory)
 for dx in [-1.10,1.10]:box('Window pilaster',(x+dx,-3.78,1.9),(.34,.24,3.8),ivory)
 for dx in [-.89,.89]:box('Sash upright',(x+dx,-3.81,1.97),(.055,.09,2.42),ivory,.006)
 for z in [.77,1.98,3.18]:box('Window sash rail',(x,-3.81,z),(1.82,.09,.045),ivory,.005)
 box('Window central mullion',(x,-3.81,1.97),(.035,.07,2.42),ivory,.003)
 box('Window sill',(x,-3.69,.76),(2.01,.34,.08),ivory)
 for side in [-1,1]:
  # Narrow hanging folds frame the glass instead of blocking the garden.
  for j in range(8):
   cx=x+side*(.94+j*.044)
   o=cyl('Damask curtain fold',(cx,-3.54+math.sin(j*1.8)*.045,1.87),.061,2.78,cloth,12);o.scale.y=.60
  box('Curtain tieback',(x+side*1.095,-3.47,1.40),(.37,.06,.055),gold)
 box('Upholstered pelmet',(x,-3.49,3.22),(2.52,.26,.25),cloth,.03)
 box('Pelmet braid',(x,-3.345,3.13),(2.51,.018,.025),gold,.002)

GROUP='Hall_and_Doors'
for y in [-3.82,-1.38]:box('Vestibule wall',(-7.35,y,1.9),(1.8,.16,3.8),cream)
box('Hall ceiling',(-7.35,-2.6,3.83),(1.8,2.5,.1),ivory)
box('Oval Office door',(-8.18,-2.6,1.45),(.12,1.75,2.9),wood)
for y in [-3.50,-1.70]:box('Door casing',(-8.07,y,1.51),(.16,.13,3.03),ivory)
box('Door casing lintel',(-8.07,-2.6,3.01),(.16,1.94,.13),ivory)
for y in [-3.04,-2.17]:
 for z in [.68,1.95]:box('Door raised panel',(-8.095,y,z),(.07,.66,.97),edge)
 cyl('Brass door handle',(-8.02,y+(.20 if y<-2.6 else -.20),1.0),.03,.19,gold).rotation_euler.y=math.pi/2
panel_image('Oval Office door seal',(-8.002,-2.6,2.42),.39,.39,seal,(math.pi/2,0,math.pi/2))
text('Hall sign','OVAL OFFICE',(-7.95,-2.6,2.97),.12,gold,(math.pi/2,0,math.pi/2))
for y in [-3.55,-1.65]:box('Cabinet doorway casing',(-6.39,y,1.51),(.13,.14,3.02),ivory)
box('Cabinet doorway lintel',(-6.39,-2.6,3.0),(.14,2.04,.14),ivory)

GROUP='Conference_Table'
# Oval mahogany table: rim, inset top, brass stringing, three substantial pedestals.
for z,rx,ry,h,m in [(.735,4.45,1.17,.13,edge),(.804,4.42,1.14,.026,gold),(.825,4.40,1.12,.028,wood)]:
 o=cyl('Oval conference table',(0,0,z),1,h,m,160);o.scale.x=rx;o.scale.y=ry
for x in [-2.8,0,2.8]:
 box('Pedestal foot',(x,0,.10),(1.35,1.42,.16),edge,.055)
 box('Pedestal column',(x,0,.40),(.72,.75,.55),wood,.075)
 box('Pedestal capital',(x,0,.65),(1.20,1.1,.12),edge,.025)
# Thin inlaid oval as a mesh ribbon, just inside the polished perimeter.
vs=[];fs=[]
for i in range(160):
 t=2*math.pi*i/160
 for rx,ry in [(4.26,1.01),(4.247,.997)]:vs.append((rx*math.cos(t),ry*math.sin(t),.843))
for i in range(160):j=(i+1)%160;fs.append((2*i,2*j,2*j+1,2*i+1))
me=bpy.data.meshes.new('Inlay');me.from_pydata(vs,[],fs);o=bpy.data.objects.new('Brass perimeter inlay',me);bpy.context.collection.objects.link(o);finish(o,o.name,gold)

GROUP='Cabinet_Chairs'
def chair(x,y,angle,president=False):
 # All parts first in local space, facing -Y. Rotate toward the table.
 before=set(bpy.context.scene.objects);height=1.44 if president else 1.21
 box('Mahogany seat frame',(0,0,.45),(.64,.63,.13),edge,.035)
 box('Leather seat cushion',(0,-.025,.535),(.60,.58,.13),leather,.052)
 box('Carved chair back',(0,.265,(.58+height)/2),(.68,.13,height-.58),wood,.065)
 box('Padded leather back',(0,.18,(.61+height)/2),(.56,.105,height-.65),leather,.049)
 for xx in [-.265,.265]:
  for yy in [-.235,.235]:box('Tapered chair leg',(xx,yy,.225),(.065,.065,.45),edge,.014)
  box('Mahogany arm',(xx,.005,.76),(.085,.57,.09),wood,.029)
  box('Arm support',(xx,-.18,.625),(.045,.045,.23),edge)
 for xx in [-.16,0,.16]:
  for z in [.81,1.02]:ball('Leather tuft button',(xx,.121,z),(.014,.01,.014),gold)
 box('Brass chair identification plate',(0,.338,.93),(.40,.014,.105),gold,.012)
 for o in set(bpy.context.scene.objects)-before:
  px,py,pz=o.location;o.location=(x+math.cos(angle)*px-math.sin(angle)*py,y+math.sin(angle)*px+math.cos(angle)*py,pz);o.rotation_euler.z+=angle
for x in [-3,-2,-1,0,1,2,3]:
 for y in [-1.73,1.73]:chair(x,y,math.pi if y<0 else 0,x==0 and y<0)
chair(-4.82,0,math.pi/2);chair(4.82,0,-math.pi/2)

GROUP='Table_Details'
for x in [-3,-2,-1,0,1,2,3]:
 for side in [-1,1]:
  y=side*.73
  box('Leather writing pad',(x,y,.856),(.62,.44,.014),leather,.018)
  box('Cabinet briefing folder',(x-.06,y,.869),(.25,.33,.012),wood,.006)
  box('Meeting agenda',(x+.12,y,.881),(.24,.31,.003),paper,.001)
  for k in range(7):box('Agenda printed line',(x+.12,y-.08+k*.023,.884),(.17 if k else .12,.004,.001),black,0)
  rod('Gold pen',(x+.28,y-.15,.884),(x+.28,y+.05,.884),.005,gold)
  cyl('Water tumbler',(x-.37,y,.91),.034,.12,water,24)
  cyl('Glass coaster',(x-.37,y,.85),.044,.004,edge,24)
  cyl('Microphone base',(x,side*.40,.865),.061,.018,black,24)
  rod('Gooseneck microphone',(x,side*.40,.87),(x,side*.47,1.04),.006,black)
  ball('Microphone capsule',(x,side*.47,1.05),(.013,.022,.018),black)
# Name cards face the walking aisles. Four active seats and presidential briefing seat.
for x,y,body in [(-2,-1.02,'MAYA CHEN'),(2,-1.02,'ELENA RUIZ'),(-2,1.02,'DANIEL BROOKS'),(2,1.02,'SAMIRA BELL'),(0,-1.02,'THE PRESIDENT')]:
 box('Engraved brass nameplate',(x,y,.93),(.78,.045,.15),gold,.008)
 text('Cabinet seat '+body,body,(x,y+(.029 if y>0 else -.029),.916),.060,black,(math.pi/2,0,math.pi if y>0 else 0))
# Table centre arrangement, low enough for sightlines.
cyl('Flowers silver bowl',(0,0,.95),.18,.17,gold,48)
for i in range(17):
 t=random.random()*math.tau;r=random.random()*.22
 ball('White cabinet roses',(r*math.cos(t),r*math.sin(t),1.07+random.random()*.12),(.07,.06,.065),rose)

GROUP='Fireplace_and_Art'
box('Marble hearth',(6.02,0,.05),(.9,2.1,.10),stone)
box('Fireplace black opening',(6.39,0,.60),(.07,1.37,1.08),black)
for y in [-.81,.81]:
 box('Mantel column',(6.25,y,.66),(.36,.24,1.31),stone)
 for z in [.14,1.13]:box('Mantel column capital',(6.21,y,z),(.43,.32,.12),ivory)
box('Carved mantel lintel',(6.25,0,1.30),(.39,1.98,.28),stone)
box('Mantel shelf',(6.19,0,1.47),(.59,2.24,.12),ivory)
for y in [-.6,-.3,0,.3,.6]:rod('Fireplace grate',(6.10,y,.17),(6.10,y,.50),.014,black)
frame('Washington above the mantel',(6.37,0,2.48),1.05,1.40,wash,'end')
frame('American landscape',(-3.65,3.69,2.25),1.60,1.12,land)
frame('Liberty portrait',(0,3.69,2.25),.96,1.35,liberty)
frame('River valley',(3.65,3.69,2.25),1.60,1.12,land)
# Sideboard with framed seal, porcelain lamps and a row of Cabinet binders.
box('Mahogany sideboard',(0,3.28,.46),(2.52,.66,.88),wood,.04)
box('Sideboard top',(0,3.28,.93),(2.62,.74,.10),edge,.025)
for x in [-.85,0,.85]:
 box('Sideboard door',(x,2.935,.49),(.73,.055,.68),edge)
 ball('Brass cupboard handle',(x+.24,2.89,.54),(.025,.017,.025),gold)
for x in [-.98,.98]:
 cyl('Porcelain lamp foot',(x,3.27,1.01),.12,.06,gold)
 ball('Porcelain lamp body',(x,3.27,1.20),(.10,.10,.22),ivory)
 bpy.ops.mesh.primitive_cone_add(vertices=48,radius1=.22,radius2=.13,depth=.29,location=(x,3.27,1.50));finish(bpy.context.object,'Linen lampshade',cloth)
# Flags with geometric folds and correct front UVs.
for y,material in [(-2.70,flag),(2.70,pflag)]:
 cyl('Flag stand',(5.87,y,.045),.22,.09,gold,48);cyl('Flag pole',(5.87,y,1.58),.018,3.03,gold)
 ball('Flag finial',(5.87,y,3.12),(.044,.044,.09),gold)
 verts=[];faces=[];nx=24;nz=20
 for j in range(nz+1):
  for i in range(nx+1):
   a=i/nx;b=j/nz;verts.append((5.87+.09*math.sin(a*math.tau*2.5+b*.5),y+(.90*a if y<0 else -.90*a),1.45+1.35*b-.13*a))
 for j in range(nz):
  for i in range(nx):k=j*(nx+1)+i;faces.append((k,k+1,k+nx+2,k+nx+1))
 me=bpy.data.meshes.new('Flag cloth');me.from_pydata(verts,[],faces);me.uv_layers.new()
 for poly in me.polygons:
  poly.use_smooth=True
  for li in poly.loop_indices:
   vi=me.loops[li].vertex_index;me.uv_layers.active.data[li].uv=(vi%(nx+1)/nx,vi//(nx+1)/nz)
 o=bpy.data.objects.new('Ceremonial flag',me);bpy.context.collection.objects.link(o);finish(o,o.name,material)

GROUP='Rose_Garden'
box('Rose Garden lawn',(0,-9,-.12),(35,13,.18),grass,0)
box('Garden path',(0,-5.8,-.012),(17,1.12,.03),stone,0)
for y in [-5.0,-6.65]:
 box('Low clipped hedge',(0,y,.38),(15,.54,.76),green,.22)
 for x in range(-7,8):
  for j in range(3):ball('Garden rose',(x+random.uniform(-.3,.3),y+random.uniform(-.19,.19),.79+random.random()*.1),(.06,.06,.08),rose)
for x in range(-11,12,3):
 rod('Tree trunk',(x,-12,0),(x,-12,4),.15,edge)
 for j in range(6):ball('Garden tree canopy',(x+random.uniform(-.9,.9),-12+random.uniform(-.8,.8),3.8+random.random()*1.8),(1.2,1.1,1.2),green)

GROUP='Lighting_Fixtures'
for x in [-3.7,0,3.7]:
 cyl('Ceiling light rosette',(x,0,3.73),.27,.07,ivory,48)
 cyl('Pendant stem',(x,0,3.35),.025,.66,gold)
 cyl('Pendant bowl rim',(x,0,3.05),.44,.08,gold,64)
 ball('Frosted pendant bowl',(x,0,3.04),(.43,.43,.14),ivory)
 for i in range(6):
  t=i*math.tau/6;rod('Pendant suspension',(x+.38*math.cos(t),.38*math.sin(t),3.08),(x+.12*math.cos(t),.12*math.sin(t),3.55),.007,gold)

# Renderable Blender lighting and a visitor-height camera.
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.world=bpy.data.worlds.new('Daylight');scene.world.use_nodes=True;scene.world.node_tree.nodes.get('Background').inputs[0].default_value=(.56,.68,.82,1);scene.world.node_tree.nodes.get('Background').inputs[1].default_value=.35
for x in [-5,-2.5,0,2.5,5]:
 d=bpy.data.lights.new('Garden daylight','AREA');d.energy=450;d.shape='RECTANGLE';d.size=2;d.size_y=2.5;o=bpy.data.objects.new(d.name,d);scene.collection.objects.link(o);o.location=(x,-3.5,2.5);o.rotation_euler=(Vector((x,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
for x in [-3.7,0,3.7]:
 d=bpy.data.lights.new('Warm ceiling wash','AREA');d.energy=230;d.size=3;o=bpy.data.objects.new(d.name,d);scene.collection.objects.link(o);o.location=(x,0,3.55)
d=bpy.data.cameras.new('Cabinet Room view');cam=bpy.data.objects.new(d.name,d);scene.collection.objects.link(cam);cam.location=(-5.78,-2.70,1.72);cam.rotation_euler=(Vector((2.2,.1,1.38))-cam.location).to_track_quat('-Z','Y').to_euler();d.lens=23;scene.camera=cam
scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(S/'Renders'/'Cabinet_Room.png')
scene.view_settings.view_transform='AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(S/'Cabinet_Room.blend'))
bpy.ops.file.make_paths_relative()
bpy.ops.wm.save_as_mainfile(filepath=str(S/'Cabinet_Room.blend'))

# Export Blender axes in centimetres. Unreal FBX OBJ import reverses Y and winding.
groups={}
for o in scene.objects:
 if o.type in {'MESH','FONT','CURVE'}:groups.setdefault(o.get('export_group','Details'),[]).append(o)
deps=bpy.context.evaluated_depsgraph_get();manifest=[]
for name,objects in groups.items():
 count=0;offset=1;used=set()
 with (S/'Meshes'/(name+'.obj')).open('w') as f:
  f.write('mtllib Cabinet.mtl\no '+name+'\n')
  for o in objects:
   eo=o.evaluated_get(deps);me=eo.to_mesh();me.calc_loop_triangles();wm=o.matrix_world;nm=wm.to_3x3().inverted().transposed();uv=me.uv_layers.active;last=None
   for tri in me.loop_triangles:
    mid=me.materials[tri.material_index].name;used.add(mid)
    if mid!=last:f.write('usemtl '+mid+'\n');last=mid
    for li in tri.loops:
     v=wm@me.vertices[me.loops[li].vertex_index].co;n=(nm@me.corner_normals[li].vector).normalized();t=uv.data[li].uv if uv else (0,0)
     f.write('v %.4f %.4f %.4f\nvt %.5f %.5f\nvn %.5f %.5f %.5f\n'%(v.x*100,v.y*100,v.z*100,t[0],t[1],n.x,n.y,n.z))
    f.write('f '+' '.join('%d/%d/%d'%(i,i,i) for i in range(offset,offset+3))+'\n');offset+=3;count+=1
   eo.to_mesh_clear()
 manifest.append({'name':name,'triangles':count,'materials':sorted(used)});print('EXPORTED',name,count,flush=True)
(S/'manifest.json').write_text(json.dumps({'groups':manifest,'materials':list(metadata.values()),'coordinates':'UE centimetres X=Blender X, Y=-Blender Y, Z=Blender Z'},indent=2))
(S/'Meshes'/'Cabinet.mtl').write_text(''.join('newmtl '+m['id']+'\nKd '+' '.join(map(str,m['color']))+'\nd 1\n\n' for m in metadata.values()))
(S/'Meshes'/'Cabinet.mtl').write_text((S/'Meshes'/'Cabinet.mtl').read_text().rstrip()+'\n')
print('CABINET_SOURCE_COMPLETE',sum(g['triangles'] for g in manifest),flush=True)
# Render is optional: the native Unreal level is the primary playtest surface.
import sys
if '--render' in sys.argv:bpy.ops.render.render(write_still=True)
