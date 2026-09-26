"""Blender-authored, public-tour-inspired West Wing expansion, in metres.
Run: blender -b --python west-wing/build_west_wing.py [-- --render]
The game exports the extension; the editable .blend also contains the original Oval.
"""
import bpy, math, random, json, sys
from pathlib import Path
from mathutils import Vector, Matrix
from math import pi, sin, cos
P=Path(__file__).resolve().parent; ROOT=P.parent
P.mkdir(exist_ok=True); (P/'renders').mkdir(exist_ok=True)
random.seed(67)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
S=bpy.context.scene; S.unit_settings.system='METRIC'
S['Scope']='Public-tour-inspired West Wing main floor, closed staff offices, upper-story exterior, colonnade and Rose Garden. Dimensions adapted for the game.'
solids=[]; floors=[]; doors=[]; zones=[]; lamps=[]
def collection(name):
 global C
 C=bpy.data.collections.new(name);S.collection.children.link(C);return C
collection('01 | Architecture and connected corridors')
def material(name,color,rough=.65,metal=0,image=None):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough;bs.inputs['Metallic'].default_value=metal
 if image:
  t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(image),check_existing=True);m.node_tree.links.new(t.outputs['Color'],bs.inputs['Base Color'])
 return m
cream=material('WW | warm ivory plaster',(.73,.70,.61),.84)
white=material('WW | enamel joinery',(.86,.84,.76),.34)
stone=material('WW | pale limestone',(.65,.62,.53),.68)
darkstone=material('WW | stone border',(.22,.24,.24),.46)
gold=material('WW | brushed brass',(.53,.35,.12),.28,.68)
wood=material('WW | mahogany',(.18,.05,.025),.31,image=ROOT/'air-force-one/textures/walnut.jpg')
oak=material('WW | floor oak',(.30,.16,.055),.46,image=ROOT/'air-force-one/textures/walnut.jpg')
leather=material('WW | cognac leather',(.17,.085,.04),.56,image=ROOT/'air-force-one/textures/cognac-leather.jpg')
green=material('WW | green leather',(.028,.079,.067),.5)
navy=material('WW | press blue',(.028,.062,.13),.84)
ink=material('WW | black fittings',(.018,.025,.031),.54)
paper=material('WW | paper and linen',(.88,.85,.75),.86)
carpet=material('WW | merlot wool carpet',(.19,.028,.035),.94)
sage=material('WW | sage upholstery',(.23,.28,.23),.95)
grass=material('WW | clipped lawn',(.10,.21,.063),1)
leaf=material('WW | boxwood',(.038,.13,.048),.95)
leaf2=material('WW | garden foliage',(.065,.22,.075),.92)
bark=material('WW | tree bark',(.11,.066,.036),.93)
rose=material('WW | ivory roses',(.91,.79,.65),.86)
pink=material('WW | blush roses',(.65,.24,.29),.86)
soil=material('WW | planting soil',(.06,.038,.022),1)
lightmat=material('WW | luminous lamp',(.95,.84,.57),.4)
bs=lightmat.node_tree.nodes.get('Principled BSDF');bs.inputs['Emission Color'].default_value=(1,.78,.48,1);bs.inputs['Emission Strength'].default_value=.8
glass=material('WW | glazing',(.57,.72,.77),.18)
glass.diffuse_color=(.57,.72,.77,.10);bs=glass.node_tree.nodes.get('Principled BSDF');bs.inputs['Alpha'].default_value=.10
gallery=[material('WW | artwork '+str(i),(1,1,1),.8,image=ROOT/path) for i,path in enumerate([
 'Unreal Projects/OvalOffice/Prototype/assets/river-landscape.png',
 'Unreal Projects/OvalOffice/Prototype/assets/washington-portrait.png',
 'Unreal Projects/OvalOffice/Prototype/assets/memories/coastal-memory.png',
 'Unreal Projects/OvalOffice/Prototype/assets/memories/volunteer-day.png'])]
seal=material('WW | presidential emblem',(1,1,1),.6,image=ROOT/'air-force-one/textures/presidential-seal.png')
def move(o):
 for c in list(o.users_collection):c.objects.unlink(o)
 C.objects.link(o);return o
def mesh(name,verts,faces,mat,uv=None):
 me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update();o=bpy.data.objects.new(name,me);C.objects.link(o);me.materials.append(mat)
 if uv:
  layer=me.uv_layers.new()
  for p in me.polygons:
   for li in p.loop_indices:layer.data[li].uv=uv[me.loops[li].vertex_index]
 return o
def box(name,loc,size,mat,bevel=0,solid=False):
 x,y,z=[d/2 for d in size];v=[(-x,-y,-z),(-x,-y,z),(-x,y,-z),(-x,y,z),(x,-y,-z),(x,-y,z),(x,y,-z),(x,y,z)]
 faces=[(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)]
 o=mesh(name,v,[tuple(reversed(f)) for f in faces],mat);o.location=loc
 layer=o.data.uv_layers.new()
 for poly in o.data.polygons:
  normal=poly.normal
  for li in poly.loop_indices:
   co=o.data.vertices[o.data.loops[li].vertex_index].co
   layer.data[li].uv=(co.x,co.y) if abs(normal.z)>.5 else (co.x,co.z) if abs(normal.y)>.5 else (co.y,co.z)
 if bevel:
  mod=o.modifiers.new('Soft architectural edges','BEVEL');mod.width=bevel;mod.segments=2
  o.modifiers.new('Corner normals','WEIGHTED_NORMAL')
 if solid:solids.append({'name':name,'rect':[loc[0]-x,loc[1]-y,loc[0]+x,loc[1]+y]})
 return o
def cylinder(name,loc,r,h,mat,vertices=20):
 v=[(r*cos(2*pi*i/vertices),r*sin(2*pi*i/vertices),z) for z in [-h/2,h/2] for i in range(vertices)]
 f=[tuple(range(vertices-1,-1,-1)),tuple(range(vertices,vertices*2))]+[(i,(i+1)%vertices,(i+1)%vertices+vertices,i+vertices) for i in range(vertices)]
 o=mesh(name,v,f,mat);o.location=loc
 for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
 return o
def sphere(name,loc,scale,mat):
 import bmesh
 me=bpy.data.meshes.new(name);bm=bmesh.new();bmesh.ops.create_uvsphere(bm,u_segments=12,v_segments=8,radius=1);bm.to_mesh(me);bm.free();o=bpy.data.objects.new(name,me);C.objects.link(o);o.location=loc;o.scale=scale;me.materials.append(mat)
 for p in me.polygons:p.use_smooth=True
 return o
def text(name,body,loc,size=.16,mat=gold,angle=0):
 cu=bpy.data.curves.new(name,'FONT');cu.body=body;cu.align_x='CENTER';cu.size=size;cu.extrude=.001;cu.resolution_u=2
 o=bpy.data.objects.new(name,cu);C.objects.link(o);o.location=loc;o.rotation_euler=(pi/2,0,angle);cu.materials.append(mat);return o
def group(before,loc=(0,0,0),angle=0):
 M=Matrix.Translation(Vector(loc))@Matrix.Rotation(angle,4,'Z')
 # Fresh objects have not evaluated matrix_world yet. Compose from their authored
 # transforms, otherwise chair backs, frame rails and lettering collapse to origin.
 for o in set(S.objects)-before:o.matrix_world=M@Matrix.LocRotScale(o.location,o.rotation_euler.to_quaternion(),o.scale)
def art(loc,w,h,index=0,angle=0):
 before=set(S.objects)
 box('Gallery backing',(0,0,0),(w+.15,.05,h+.15),wood,.01)
 mesh('Framed public-room artwork',[(-w/2,-.031,-h/2),(w/2,-.031,-h/2),(w/2,-.031,h/2),(-w/2,-.031,h/2)],[(0,1,2,3)],gallery[index],[(0,0),(1,0),(1,1),(0,1)])
 for x in [-w/2-.035,w/2+.035]:box('Gilt vertical rail',(x,-.06,0),(.07,.055,h+.16),gold,.006)
 for z in [-h/2-.035,h/2+.035]:box('Gilt horizontal rail',(0,-.06,z),(w+.15,.055,.07),gold,.006)
 group(before,loc,angle)
def plaque(body,loc,angle=0,w=1.4):
 before=set(S.objects);box('Room name plaque',(0,0,0),(w,.035,.30),gold,.015);text('Room name',body,(0,-.025,-.048),.11,ink);group(before,loc,angle)
def floor(name,r,mat=oak,ceiling=True):
 x1,y1,x2,y2=r;floors.append(r);box(name+' floor',((x1+x2)/2,(y1+y2)/2,-.09),(x2-x1,y2-y1,.18),mat)
 if ceiling:box('ROOF__'+name,((x1+x2)/2,(y1+y2)/2,3.48),(x2-x1,y2-y1,.18),white)
def wall(name,a,b,openings=(),windows=(),height=3.4,mat=cream,thick=.18):
 # Openings are along the wall axis: center, width. Window sills are 0.85 m.
 ax,ay=a;bx,by=b;length=math.hypot(bx-ax,by-ay);vertical=abs(bx-ax)<.01
 def seg(t0,t1,z0,z1,label,ma=mat,depth=thick,collision=False):
  if t1-t0<.001:return
  f=((t0+t1)/2)/length;x=ax+(bx-ax)*f;y=ay+(by-ay)*f
  return box(label,(x,y,(z0+z1)/2),(depth,t1-t0,z1-z0) if vertical else (t1-t0,depth,z1-z0),ma,0,collision)
 ranges=sorted([(c-w/2,c+w/2,'door') for c,w in openings]+[(c-w/2,c+w/2,'window') for c,w in windows]);start=0
 for lo,hi,kind in ranges+[(length,length,'end')]:
  if lo>start:
   seg(start,lo,0,height,name,collision=True)
   seg(start,lo,.08,.17,'Skirting',white,thick+.055);seg(start,lo,.90,.97,'Chair rail',white,thick+.045)
   seg(start,lo,3.20,3.34,'Crown moulding',white,thick+.12)
   for pos in [start+.45+j*.80 for j in range(int((lo-start)/.8))]:
    seg(max(start+.04,pos-.34),min(lo-.04,pos+.34),.27,.72,'Raised wainscot panel',white,thick+.022)
  if kind=='door':
   seg(lo,hi,2.53,height,'Door lintel');seg(lo-.075,lo+.015,0,2.58,'Door casing',white,thick+.09);seg(hi-.015,hi+.075,0,2.58,'Door casing',white,thick+.09);seg(lo-.07,hi+.07,2.53,2.65,'Door pediment',white,thick+.09)
   f=((lo+hi)/2)/length;doors.append({'name':name,'point':[ax+(bx-ax)*f,ay+(by-ay)*f],'width':hi-lo})
  elif kind=='window':
   seg(lo,hi,0,.85,'Window apron',collision=True);seg(lo,hi,2.92,height,'Window head')
   seg(lo-.055,lo+.03,.83,3.0,'Window jamb',white,thick+.06);seg(hi-.03,hi+.055,.83,3.0,'Window jamb',white,thick+.06)
   seg(lo-.06,hi+.06,.80,.88,'Window sill',white,thick+.16);seg(lo,hi,2.90,3.0,'Window cornice',white,thick+.1)
   seg(lo,hi,.90,2.88,'WW_WINDOW_GLASS',glass,.018)
   seg((lo+hi)/2-.025,(lo+hi)/2+.025,.88,2.94,'Window center stile',white,.06)
   for zz in [1.57,2.22]:seg(lo,hi,zz-.018,zz+.018,'Window sash bar',white,.06)
  start=hi
def closed_door(name,loc,angle=0):
 before=set(S.objects);box('Closed staff door',(0,0,1.27),(1.08,.08,2.5),white,.016)
 for z,h in [(.63,.87),(1.78,1.0)]:box('Inset door panel',(0,-.05,z),(.78,.025,h),cream,.01)
 sphere('Brass door handle',(.40,-.10,1.05),(.04,.035,.04),gold);plaque(name,(0,-.063,2.84),0,1.55);group(before,loc,angle)
def pointlight(name,loc,power=90,color=(1,.85,.65),radius=.45):
 d=bpy.data.lights.new(name,'POINT');d.energy=power;d.color=color;d.shadow_soft_size=radius;o=bpy.data.objects.new(name,d);C.objects.link(o);o.location=loc;lamps.append({'position':list(loc),'power':power})
def ceiling_light(x,y):
 cylinder('Ceiling brass rosette',(x,y,3.37),.28,.065,gold)
 cylinder('Opal glass ceiling light',(x,y,3.27),.235,.15,lightmat)
 pointlight('Warm ceiling pool',(x,y,2.95),120)
def sconce(x,y,angle=0):
 before=set(S.objects);box('Sconce backplate',(0,0,2.1),(.12,.055,.27),gold,.018);cylinder('Sconce stem',(0,-.14,2.12),.013,.28,gold);cylinder('Sconce linen shade',(0,-.14,2.29),.115,.20,lightmat);group(before,(x,y,0),angle)
def chair(loc,angle=0,mat=leather,name='Executive chair'):
 before=set(S.objects)
 box(name+' seat',(0,0,.47),(.59,.56,.13),mat,.05);box(name+' back',(0,.245,.88),(.59,.13,.79),mat,.06)
 for x in [-.245,.245]:
  for y in [-.205,.205]:box('Chair leg',(x,y,.22),(.045,.045,.43),wood,.009)
  box('Chair arm',(x*1.18,0,.69),(.065,.59,.065),wood,.018)
 group(before,loc,angle)
 solids.append({'name':name,'rect':[loc[0]-.34,loc[1]-.34,loc[0]+.34,loc[1]+.36]})
def table(name,loc,w,l):
 x,y=loc;box(name+' top',(x,y,.79),(w,l,.13),wood,.16,True);box('Table edge inlay',(x,y,.797),(w-.08,l-.08,.10),gold,.13)
 box('Polished table surface',(x,y,.849),(w-.10,l-.10,.026),wood,.13)
 for off in [-l*.28,l*.28]:box('Table pedestal',(x,y+off,.39),(.80,.67,.68),wood,.08);box('Table pedestal foot',(x,y+off,.10),(1.10,.94,.14),wood,.06)
def folio(x,y,angle=0):
 before=set(S.objects);box('Briefing folder',(0,0,.88),(.28,.36,.022),navy,.006);box('Briefing paper',(0,0,.893),(.25,.32,.003),paper)
 for j in range(6):box('Printed briefing line',(0,-.09+j*.033,.896),(.17,.002,.001),ink)
 group(before,(x,y,0),angle)
def planter(x,y,r=.24):
 solids.append({'name':'Planter','rect':[x-r,y-r,x+r,y+r]})
 cylinder('Ceramic planter',(x,y,.25),r,.5,white);cylinder('Planter soil',(x,y,.505),r*.94,.012,soil)
 for i in range(13):
  a=i*2.4;sphere('Planter broad leaf',(x+cos(a)*r*.65,y+sin(a)*r*.65,.63+random.random()*.35),(.09,.20,.11),leaf if i%2 else leaf2)
def sofa(loc,angle=0):
 before=set(S.objects);box('Reception sofa base',(0,0,.30),(2.05,.80,.42),sage,.09);box('Reception sofa back',(0,.30,.72),(2.05,.20,.73),sage,.07)
 for x in [-.66,0,.66]:box('Individual sofa cushion',(x,-.02,.54),(.64,.68,.13),sage,.055)
 for x in [-1,1]:box('Sofa rolled arm',(x,0,.62),(.20,.82,.41),sage,.075)
 group(before,loc,angle)
 dx=abs(cos(angle))*1.12+abs(sin(angle))*.44;dy=abs(sin(angle))*1.12+abs(cos(angle))*.44
 solids.append({'name':'Sofa','rect':[loc[0]-dx,loc[1]-dy,loc[0]+dx,loc[1]+dy]})
def room_zone(id,name,r,spawn,target,page=None):zones.append({'id':id,'name':name,'rect':r,'spawn':spawn,'target':target,'page':page})
# Main accessible circulation: a generous 3 m loop around the Roosevelt Room.
floor('East gallery',[1.5,1.5,4.5,20],stone)
floor('West gallery',[-10,1.5,-7,20],stone)
floor('South crosshall',[-7,1.5,1.5,4.5],stone)
floor('North crosshall',[-7,12.5,1.5,15.5],stone)
floor('North reception',[-10,20,4.5,25],stone)
for x in [3,-8.5]:box('Hall runner',(x,10.75,.011),(1.45,18.4,.015),carpet)
for y in [3,14]:box('Crosshall runner',(-2.75,y,.016),(13.3,1.48,.015),carpet)
for x in [3,-8.5]:
 for y in [3,7.2,11.5,16,21.8]:ceiling_light(x,y)
for x in [-5,-1]:
 for y in [3,14,22.5]:ceiling_light(x,y)
# Staff offices form the outer mass, with named closed doors.
box('Staff office block',(-13.5,9.6,1.7),(6.96,30.76,3.4),cream,solid=True)
for y,label in [(3,'CHIEF OF STAFF'),(8,'SENIOR ADVISERS'),(13,'COMMUNICATIONS'),(18,'WEST WING STAFF'),(23,'VICE PRESIDENT')]:
 closed_door(label,(-9.88,y,0),pi/2)
for y in [5.4,10.4,15.4,20.4]:art((-9.78,y,2.0),1.05,.70,2,pi/2)
box('North staff office suite',(-2.75,17.75,1.7),(8.5,4.5,3.4),cream,solid=True)
for x,label in [(-5,'STAFF SECRETARY'),(-1,'POLICY COUNCIL')]:closed_door(label,(x,15.38,0));closed_door('PRIVATE OFFICE',(x,20.12,0),pi)
wall('West gallery south end',(-10,1.5),(-7,1.5))
wall('Staff western facade',(-17,-5.8),(-17,25),windows=[(3+i*4.4,1.5) for i in range(6)])
wall('South staff facade',(-17,-5.8),(1.5,-5.8),windows=[(2+i*3.4,1.5) for i in range(5)])
wall('North entrance facade',(-17,25),(4.5,25),openings=[(18.5,1.8)],windows=[(3,1.5),(7,1.5),(11,1.5)])
closed_door('WEST WING ENTRANCE',(1.5,25,0),pi)
room_zone('hall','West Wing galleries',[-10,1.5,4.5,20],[3,3],[3,12])
# The Roosevelt Room: warm paneling, green leather and a fireplace.
collection('02 | Roosevelt Room')
floor('Roosevelt Room',[-7,4.5,1.5,12.5],oak)
wall('Roosevelt east wall',(1.5,4.5),(1.5,12.5),openings=[(4,1.8)],mat=wood)
wall('Roosevelt west wall',(-7,4.5),(-7,12.5),openings=[(4,1.8)],mat=wood)
wall('Roosevelt south wall',(-7,4.5),(1.5,4.5),mat=wood)
wall('Roosevelt north wall',(-7,12.5),(1.5,12.5),mat=wood)
plaque('ROOSEVELT ROOM',(1.61,7.1,2.75),pi/2,1.7)
box('Roosevelt bordered rug',(-2.75,8.5,.017),(6.7,6.5,.028),gold,.02)
box('Roosevelt rug inset',(-2.75,8.5,.034),(6.55,6.35,.018),carpet,.015)
table('Roosevelt conference table',(-2.75,8.6),2.15,4.45)
for y in [7.1,8.1,9.1,10.1]:
 chair((-4.23,y,0),pi/2,green);chair((-1.27,y,0),-pi/2,green);folio(-3.35,y);folio(-2.15,y,pi)
chair((-2.75,5.78,0),pi,green);chair((-2.75,11.40,0),0,green)
for y in [6.7,10.3]:ceiling_light(-2.75,y)
box('Roosevelt fireplace surround',(-2.75,12.30,.67),(1.9,.3,1.30),stone,.025,True)
box('Roosevelt firebox',(-2.75,12.12,.54),(1.32,.06,.88),ink)
box('Roosevelt mantel',(-2.75,12.26,1.37),(2.17,.47,.14),white,.018)
art((-2.75,12.34,2.43),.91,1.02,1)
for x in [-5.6,.25]:art((x,4.63,2.03),1.05,.73,0,pi)
room_zone('roosevelt','Roosevelt Room',[-7,4.5,1.5,12.5],[-.05,8.5],[-2.75,8.5],'congress')
# Cabinet Room overlooks the garden with large multi-light windows.
collection('03 | Cabinet Room')
floor('Cabinet Room',[4.5,7,13,20],oak)
wall('Cabinet west wall',(4.5,7),(4.5,20),openings=[(3.5,1.9)])
wall('Cabinet garden wall',(13,7),(13,20),openings=[(4,1.8)],windows=[(1.35,1.7),(7.25,1.8),(10.6,1.8)])
wall('Cabinet north wall',(4.5,20),(13,20))
art((8.75,19.86,2.12),1.85,1.1,0)
wall('Cabinet south wall',(4.5,7),(13,7))
plaque('CABINET ROOM',(4.38,9.15,2.76),-pi/2,1.6)
box('Cabinet rug', (8.75,13.45,.019),(6.7,11.2,.035),gold,.04)
box('Cabinet rug field',(8.75,13.45,.041),(6.48,10.98,.02),carpet,.03)
table('Cabinet mahogany table',(8.75,13.45),2.28,8.0)
for i in range(9):
 y=9.87+i*.9;chair((7.15,y,0),pi/2);chair((10.35,y,0),-pi/2);folio(8.02,y);folio(9.48,y,pi)
chair((8.75,8.90,0),pi);chair((8.75,18.05,0))
for y in [9,13.5,18]:ceiling_light(8.75,y)
art((8.75,7.13,2.16),1.02,1.10,1,pi)
for y in [8.5,18.6]:planter(12.40,y)
room_zone('cabinet','Cabinet Room',[4.5,7,13,20],[5.7,10.5],[8.75,13.4],'team')
# Private study on the southwest side of the Oval approach.
collection('04 | Private study and reception lobby')
floor('Presidential study',[-7,-5.5,1.5,1.5],oak)
wall('Study north wall',(-7,1.5),(1.5,1.5),openings=[(5.2,1.7)])
wall('Study west wall',(-7,-5.5),(-7,1.5));wall('Study east wall',(1.5,-5.5),(1.5,1.5))
wall('Study garden wall',(-7,-5.5),(1.5,-5.5),windows=[(2,1.7),(6,1.7)])
plaque('PRESIDENTIAL STUDY',(-.45,1.62,2.73),pi,1.9)
box('Study rug',(-2.75,-1.9,.018),(5.7,4.5,.03),sage,.025)
table('Private study desk',(-3.5,-3.05),1.95,.90);chair((-3.5,-3.97,0),pi);folio(-3.50,-3.05)
sofa((-.05,-2.25,0),-pi/2);art((-6.86,-1.8,2.1),1.25,.84,0,pi/2)
box('Library cabinet',(-6.64,-3.65,1.21),(.42,2.9,2.42),wood,.016,True)
for z in [.3,.8,1.3,1.8,2.3]:box('Library shelf',(-6.36,-3.65,z),(.35,2.76,.045),wood)
for i in range(28):
 y=-4.93+(i%14)*.19;z=.34+(i//14)*1.0;box('Leather bound library volume',(-6.23,y,z+.21),(.19,.12,.40),carpet if i%3 else navy,.005)
ceiling_light(-2.75,-1.9)
room_zone('study','Private study',[-7,-5.5,1.5,1.5],[-1.8,.30],[-3.5,-3.05],'journal')
sofa((-4.2,21.4,0),pi);sofa((-4.2,23.65,0));box('Lobby low table',(-4.2,22.5,.42),(1.6,.76,.12),wood,.035,True)
box('Reception desk',(0,23.9,.53),(2.1,.76,1.05),wood,.04,True)
plaque('THE WEST WING',(-1.8,24.88,2.5),0,2.8);art((-7.7,24.87,2.07),1.5,1.02,0)
for x,y in [(-9.3,24.3),(3.6,24.2)]:planter(x,y,.30)
room_zone('lobby','West Wing reception',[-10,20,4.5,25],[1.5,21.7],[-3,22.5],'appointments')
# Press extension, 49 seats, a center aisle and clear side aisles.
collection('05 | James S. Brady press briefing room')
floor('Press briefing room',[4.5,20,14,34],navy)
wall('Press west wall',(4.5,20),(4.5,34),openings=[(2.5,1.9)])
wall('Press east wall',(14,20),(14,34),windows=[(2.4,1.6),(6,1.6),(9.6,1.6)])
wall('Press north wall',(4.5,34),(14,34));wall('Press south wall',(13,20),(14,20))
plaque('PRESS BRIEFING ROOM',(4.38,23.85,2.76),-pi/2,1.9)
box('Press stage',(9.25,32.3,.10),(7.8,2.8,.20),navy,.012,True)
box('Blue briefing backdrop',(9.25,33.68,1.78),(7.8,.16,3.35),navy)
text('Press backdrop heading','THE WHITE HOUSE',(9.25,33.56,2.30),.25,paper)
text('Press backdrop subheading','WASHINGTON',(9.25,33.55,2.06),.12,paper)
# Small architectural portico icon is modeled relief, not a raster backdrop.
box('Press logo roof',(9.25,33.54,2.94),(1.8,.025,.06),paper)
for x in [8.55,8.83,9.11,9.39,9.67,9.95]:box('Press logo column',(x,33.54,2.69),(.045,.025,.44),paper)
box('Press logo base',(9.25,33.54,2.45),(1.8,.025,.05),paper)
box('Press podium',(9.25,31.87,.78),(.84,.58,1.15),wood,.035)
box('Podium reading surface',(9.25,31.86,1.38),(1.0,.69,.09),ink,.03)
disc=cylinder('Podium presidential seal',(9.25,31.565,.99),.235,.022,gold,48);disc.rotation_euler.x=pi/2
mesh('Podium emblem face',[(9.04,31.547,.78),(9.46,31.547,.78),(9.46,31.547,1.2),(9.04,31.547,1.2)],[(0,1,2,3)],seal,[(0,0),(1,0),(1,1),(0,1)])
for x in [8.97,9.53]:cylinder('Podium microphone',(x,31.95,1.57),.016,.30,ink,12)
for row in range(7):
 for col in range(7):
  x=6.3+col*.78+(1 if col>=3 else 0);y=22.65+row*1.03
  chair((x,y,0),pi,navy,'Press chair')
for x in [6.4,9.2,12.2]:
 for y in [22,26.5,30.8]:ceiling_light(x,y)
room_zone('press','Press briefing room',[4.5,20,14,34],[8.75,21.1],[8.75,32],'moments')
# Oval transition vestibule and exterior envelope. The existing room is kept intact.
collection('06 | Oval approach and colonnade')
wall('Oval vestibule side',(4.5,1.5),(4.5,7),openings=[(1.5,1.8)])
closed_door('THE OVAL OFFICE',(4.52,3,0),-pi/2)
box('Oval doorway threshold',(4.47,3,.012),(.32,1.8,.025),stone)
wall('Gallery south return',(1.5,1.5),(4.5,1.5))
for y in [5.7,11.35]:art((1.62,y,2.03),.85,.62,2,pi/2)
for y in [8.15,16.1,18.8]:art((4.38,y,2.07),.92,.65,0,-pi/2)
for x in [-5.4,-1.2]:art((x,12.62,2.07),1.1,.72,3,pi)
floor('Garden access',[13,10.1,14.5,12.0],stone,False)
floor('Garden side path',[14.5,8.3,16.5,20],stone,False)
floor('West colonnade',[13,5,36,8.3],stone,False)
box('ROOF__West colonnade',(24.5,6.65,3.36),(23.5,3.7,.20),white)
box('Colonnade south entablature',(24.5,5.05,3.11),(23.5,.32,.40),white)
box('Colonnade north entablature',(24.5,8.2,3.11),(23.5,.32,.40),white)
for x in [14.3+i*2.45 for i in range(9)]:
 for y in [5.05,8.20]:
  cylinder('Colonnade column shaft',(x,y,1.53),.15,2.80,white,24)
  cylinder('Column base',(x,y,.13),.23,.22,white,24);cylinder('Doric capital',(x,y,2.94),.23,.17,white,24)
  solids.append({'name':'Colonnade column','rect':[x-.20,y-.20,x+.20,y+.20]})
for x in [17,23,29,35]:cylinder('Colonnade lantern',(x,6.65,3.05),.14,.28,lightmat)
room_zone('colonnade','West Colonnade',[13,5,36,8.3],[17,6.6],[34,6.6])
# Rose Garden, with paths that link back to the Cabinet Room.
collection('07 | Rose Garden and landscaped exterior')
floor('Rose Garden grounds',[14.5,-10.5,36,5],grass,False)
for r in [[14.5,-8.8,16,5],[16,-9,35,-7.6],[16,3.6,35,5],[16,-7.6,17.4,3.6],[33.6,-7.6,35,3.6],[24.2,-7.6,25.8,3.6]]:
 x1,y1,x2,y2=r;box('Garden limestone path',((x1+x2)/2,(y1+y2)/2,.025),(x2-x1,y2-y1,.045),stone)
for x in [18.2,32.8]:
 solids.append({'name':'Rose border','rect':[x-.44,-7.13,x+.44,3.05]})
 box('Rose border soil',(x,-2,.02),(1.2,10.5,.035),soil)
 for j in range(24):
  y=-6.8+j*.41;sphere('Clipped garden hedge',(x,y,.38),(.38,.30,.40),leaf)
  for k in range(3):
   xx=x+random.uniform(-.35,.35);yy=y+random.uniform(-.13,.13);zz=.76+random.uniform(-.12,.16)
   sphere('Rose flower',(xx,yy,zz),(.074,.074,.052),rose if j%4 else pink)
for x in [20.4,29.5]:
 for y in [-6.5,2.0]:
  cylinder('Garden tree trunk',(x,y,1.7),.14,3.4,bark)
  for i in range(8):
   a=i*2.4;center=Vector((x+cos(a)*.72,y+sin(a)*.72,3.25+random.random()*.9))
   base=Vector((x,y,2.5));delta=center-base
   branch=cylinder('Tree crown branch',tuple((base+center)/2),.026,delta.length,bark,10);branch.rotation_euler=delta.to_track_quat('Z','Y').to_euler()
   verts=[];faces=[]
   for k in range(150):
    p=center+Vector((random.gauss(0,.48),random.gauss(0,.48),random.gauss(0,.38)))
    u=Vector((random.uniform(-1,1),random.uniform(-1,1),random.uniform(-.5,.5))).normalized()*random.uniform(.065,.13)
    v=u.cross(Vector((0,0,1))).normalized()*random.uniform(.030,.06);n=len(verts)
    verts.extend([tuple(p-u),tuple(p+v),tuple(p+u),tuple(p-v)]);faces.append((n,n+1,n+2,n+3))
   mesh('Individual garden leaves',verts,faces,leaf2 if i%2 else leaf)
  solids.append({'name':'Garden tree','rect':[x-.24,y-.24,x+.24,y+.24]})
for y in [-5.0,.5]:
 box('Garden bench seat',(35.35,y,.48),(.65,1.8,.11),white,.035,True);box('Garden bench back',(35.66,y,.86),(.08,1.8,.76),white,.02)
 for yy in [y-.65,y+.65]:box('Garden bench support',(35.35,yy,.23),(.55,.12,.46),stone,.01)
room_zone('garden','Rose Garden',[14.5,-10.5,36,5],[25,2.5],[25,-7])
# The closed upper storey and roofs complete the recognizable building mass.
collection('08 | Upper storey and external massing')
box('UPPER__West Wing second storey',(-6.5,9.6,5.17),(21,30.8,3.05),cream)
box('UPPER__West Wing roof cornice',(-6.5,9.6,6.72),(21.4,31.2,.20),white)
box('UPPER__West Wing flat roof',(-6.5,9.6,6.85),(21.6,31.4,.10),stone)
for y in [-3.0,1.5,6,10.5,15,19.5,23]:
 for x in [-17.04,4.04]:
  box('UPPER__Facade window glass',(x,y,5.25),(.025,1.35,1.63),navy)
  for yy in [y-.70,y+.70]:box('UPPER__Facade window jamb',(x,yy,5.25),(.12,.08,1.83),white)
  for zz in [4.35,6.15]:box('UPPER__Facade window rail',(x,y,zz),(.13,1.50,.08),white)
  box('UPPER__Window center',(x,y,5.25),(.15,.045,1.70),white)
  box('UPPER__Window crossbar',(x,y,5.25),(.15,1.38,.045),white)
box('Outside lawn setting',(6,9,-.17),(65,62,.14),grass)
# Oval exterior only in the extension export; its detailed interior is the existing level.
for i in range(96):
 a=2*pi*i/96;b=2*pi*(i+1)/96;mid=(a+b)/2
 # Curved facade excludes the northwestern segment buried in the main building.
 if 1.3<mid<2.7:continue
 x=8.4+4.48*cos(mid);y=0+5.49*sin(mid);width=math.hypot(4.48*(cos(b)-cos(a)),5.49*(sin(b)-sin(a)))
 o=box('OVAL_SHELL__Curved exterior',(x,y,2.7),(width,.15,5.4),cream);o.rotation_euler.z=math.atan2(5.49*cos(mid),-4.48*sin(mid))
roof=cylinder('ROOF__Oval exterior roof',(8.4,0,5.5),1,.15,white,96);roof.scale.x=4.65;roof.scale.y=5.65
# Marker data drives game collision and room shortcuts from the authored layout.
collection('09 | Player starts and navigation')
for zone in zones:
 o=bpy.data.objects.new('PLAYER_START_'+zone['id'].upper(),None);C.objects.link(o);o.location=(*zone['spawn'],.05);o.empty_display_type='ARROWS'
for name,pt in [('RETURN_OVAL',(3.6,3)),('GARDEN_DOOR',(12.4,11))]:
 o=bpy.data.objects.new(name,None);C.objects.link(o);o.location=(*pt,1.0)
nav={'radius':.28,'floors':floors,'solids':solids,'doors':doors,'zones':zones,'spawn':[3,3],'ovalPortal':[3.7,3],'scope':S['Scope']}
(P/'navigation.json').write_text(json.dumps(nav,indent=2))
# Lighting and composed Blender cameras.
collection('10 | Lighting and cameras')
world=bpy.data.worlds.new('West Wing afternoon');world.use_nodes=True;world.node_tree.nodes.get('Background').inputs[0].default_value=(.54,.68,.82,1);world.node_tree.nodes.get('Background').inputs[1].default_value=.38;S.world=world
d=bpy.data.lights.new('Afternoon sun','SUN');d.energy=2.2;d.angle=.12;sun=bpy.data.objects.new('Afternoon sun',d);C.objects.link(sun);sun.rotation_euler=(.52,-.47,-.75)
def camera(name,loc,target,lens=23):
 d=bpy.data.cameras.new(name);d.lens=lens;d.clip_start=.03;d.clip_end=200;o=bpy.data.objects.new(name,d);C.objects.link(o);o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();return o
hallcam=camera('01 | West Wing gallery',(3,3.0,1.66),(3,13.4,1.60),20)
cabinetcam=camera('02 | Cabinet Room',(5.8,8.4,1.75),(9.0,14.2,1.10),20)
presscam=camera('03 | Press briefing',(9.0,20.9,1.82),(9.25,32.0,1.25),21)
gardencam=camera('04 | Colonnade and Rose Garden',(34.0,3.0,1.7),(16.5,6.3,1.30),23)
overview=camera('05 | West Wing cutaway',(44,-40,48),(4,10,0),43)
S.camera=hallcam;S.render.engine='CYCLES';S.cycles.samples=32;S.cycles.use_denoising=True;S.cycles.max_bounces=6;S.cycles.diffuse_bounces=3
S.render.resolution_x=1440;S.render.resolution_y=900;S.render.resolution_percentage=100;S.render.image_settings.file_format='PNG';S.view_settings.view_transform='AgX';S.view_settings.look='AgX - Medium High Contrast';S.view_settings.exposure=.45
bpy.context.view_layer.update()
# Export only the authored expansion. Linked visual detail becomes a static game asset.
bpy.ops.object.select_all(action='DESELECT')
for o in S.objects:
 if o.type in {'MESH','CURVE','FONT'}:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(P/'West_Wing.glb'),export_format='GLB',use_selection=True,export_apply=True,export_texcoords=True,export_normals=True,export_yup=True,export_extras=True)
# Add an editable instance of the original Oval Office to the complete Blender file.
# The game still loads its existing polished room through the marked doorway.
with bpy.data.libraries.load(str(ROOT/'oval-office/Oval_Office.blend'),link=False) as (src,dst):
 dst.objects=[n for n in src.objects if not n.startswith(('Camera','Light','Area','Sun','Garden','Tree','Hedge','Lawn','South lawn','Distant garden','Individual garden'))]
ovalcollection=collection('11 | Original Oval Office - preserved detailed scene')
T=Matrix.Translation(Vector((8.4,0,0)))@Matrix.Rotation(pi,4,'Z')
for o in dst.objects:
 if o and o.type in {'MESH','CURVE','FONT'}:
  ovalcollection.objects.link(o)
bpy.context.view_layer.update()
for o in list(ovalcollection.objects):
 # The original presentation includes oversized terrain and studio backgrounds.
 # Keep the room and furnishings; the extension has its own exterior landscape.
 if max(o.dimensions.x,o.dimensions.y)>16:
  ovalcollection.objects.unlink(o);continue
 if o.parent is None:o.matrix_world=T@o.matrix_world
for o in S.objects:
 if o.name.startswith('OVAL_SHELL__'):o.hide_render=True;o.hide_set(True)
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(P/'West_Wing.blend'))
print('WEST_WING_BUILD_READY',len(S.objects),'objects',len(solids),'collision rectangles',flush=True)
if '--render' in sys.argv or '--render-garden' in sys.argv:
 for cam,filename in [(hallcam,'West_Wing_Gallery.png'),(cabinetcam,'Cabinet_Room.png'),(gardencam,'Colonnade_Garden.png')]:
  if '--render-garden' in sys.argv and cam!=gardencam:continue
  S.camera=cam;S.render.filepath=str(P/'renders'/filename);bpy.ops.render.render(write_still=True)
