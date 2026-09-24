import bpy, math, random, json, sys
from pathlib import Path
from mathutils import Vector, Matrix
from math import sin,cos,pi
P=Path(__file__).parent;random.seed(51)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
S=bpy.context.scene;S.unit_settings.system='METRIC'
def collection(name):
 global C
 C=bpy.data.collections.new(name);S.collection.children.link(C);return C
collection('01 | Cabin shell and curved ceiling')
def move(o):
 for c in list(o.users_collection):c.objects.unlink(o)
 C.objects.link(o);return o
def mat(name,color,rough=.5,metal=0):
 m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*color,1);bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough;bs.inputs['Metallic'].default_value=metal;return m
def image_mat(name,file,rough=.7,normal=.25):
 m=mat(name,(.5,.5,.5),rough);n=m.node_tree.nodes;l=m.node_tree.links;bs=n.get('Principled BSDF');t=n.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(P/'textures'/(file+'.jpg')));l.new(t.outputs['Color'],bs.inputs['Base Color'])
 path=P/'textures'/(file+'-normal.png')
 if path.exists():
  nt=n.new('ShaderNodeTexImage');nt.image=bpy.data.images.load(str(path));nt.image.colorspace_settings.name='Non-Color';nm=n.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=normal;l.new(nt.outputs['Color'],nm.inputs['Color']);l.new(nm.outputs[0],bs.inputs['Normal'])
 return m
ivory=image_mat('Warm ivory cabin leather','ivory-panels',.78,.025)
leather=image_mat('Cognac leather | fine grain','cognac-leather',.58,.14)
wood=image_mat('Bookmatched walnut veneer','walnut',.31,.18)
wood.node_tree.nodes.get('Principled BSDF').inputs['Coat Weight'].default_value=.22
carpet=image_mat('Presidential star wool carpet','star-carpet',.98,.65)
hallcarpet=image_mat('Blue grey corridor wool','hall-carpet',.99,.6)
trim=mat('Champagne satin metal',(.39,.32,.21),.28,.65)
dark=mat('Dark bronze reveals',(.026,.023,.021),.38,.35)
black=mat('Soft black polymer',(.009,.014,.017),.58)
green=mat('Dark green leather desk blotter',(.018,.036,.034),.58)
paper=mat('Warm stationery',(.81,.80,.74),.9)
seam=mat('Saddle stitching',(.25,.145,.07),.9)
cream=mat('Window surround enamel',(.76,.76,.69),.37)
navy=mat('Navy textile',(.012,.029,.055),.92)
glass=mat('Window glass',(.90,.95,.98),.08);glass.node_tree.nodes.get('Principled BSDF').inputs['Transmission Weight'].default_value=.94
def cube(name,loc,size,ma,bevel=.008):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=move(bpy.context.object);o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(ma)
 if ma==wood:
  layer=o.data.uv_layers.active
  for poly in o.data.polygons:
   for li in poly.loop_indices:
    co=o.data.vertices[o.data.loops[li].vertex_index].co;no=poly.normal
    layer.data[li].uv=(co.x*.65,co.y*.65) if abs(no.z)>.5 else ((co.x*.65,co.z*.65) if abs(no.y)>.5 else (co.y*.65,co.z*.65))
 if bevel:
  b=o.modifiers.new('Soft manufactured edges','BEVEL');b.width=bevel;b.segments=3;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
 return o
def mesh(name,v,f,ma,uv=None):
 me=bpy.data.meshes.new(name);me.from_pydata(v,[],f);me.update();o=bpy.data.objects.new(name,me);C.objects.link(o);me.materials.append(ma)
 if uv:
  layer=me.uv_layers.new()
  for poly in me.polygons:
   for li in poly.loop_indices:layer.data[li].uv=uv[me.loops[li].vertex_index]
 return o
def sphere(name,loc,scale,ma):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,radius=1,location=loc);o=move(bpy.context.object);o.name=name;o.scale=scale;o.data.materials.append(ma)
 for p in o.data.polygons:p.use_smooth=True
 return o
def cyl(name,loc,r,depth,ma,verts=32):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=depth,location=loc);o=move(bpy.context.object);o.name=name;o.data.materials.append(ma);b=o.modifiers.new('Edge glints','BEVEL');b.width=.004;b.segments=3;o.modifiers.new('Normals','WEIGHTED_NORMAL');return o
def curve(name,pts,r,ma,closed=False):
 cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=1;cu.bevel_depth=r;cu.bevel_resolution=2;sp=cu.splines.new('POLY');sp.points.add(len(pts)-1)
 for p,co in zip(sp.points,pts):p.co=(*co,1)
 sp.use_cyclic_u=closed;o=bpy.data.objects.new(name,cu);C.objects.link(o);cu.materials.append(ma);return o
def group(before,loc,angle=0):
 bpy.context.view_layer.update();tr=Matrix.Translation(Vector(loc))@Matrix.Rotation(angle,4,'Z')
 for o in set(bpy.data.objects)-before:o.matrix_world=tr@o.matrix_world
def plane(name,coords,uv,ma):return mesh(name,coords,[(0,1,2,3)],ma,uv)
def text(name,body,loc,size,ma,rotation=(pi/2,0,0)):
 cu=bpy.data.curves.new(name,'FONT');cu.body=body;cu.size=size;cu.align_x='CENTER';cu.extrude=.0003;o=bpy.data.objects.new(name,cu);C.objects.link(o);o.location=loc;o.rotation_euler=rotation;cu.materials.append(ma);return o
def area(name,loc,target,power,size,color=(1,.9,.75),shape='DISK',size_y=None):
 d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape=shape;d.size=size
 if size_y is not None:d.size_y=size_y
 d.color=color;o=bpy.data.objects.new(name,d);C.objects.link(o);o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();return o
def floor(name,x0,x1,y0,y1,ma):
 return plane(name,[(x0,y0,.012),(x1,y0,.012),(x1,y1,.012),(x0,y1,.012)],[(x0,y0),(x1,y0),(x1,y1),(x0,y1)],ma)
floor('Office star carpet',-2.5,1.35,0,4.65,carpet);floor('Conference star carpet',-2.5,1.35,5.2,11.65,carpet);floor('Continuous hallway runner',1.35,2.55,0,11.65,hallcarpet);floor('Connecting vestibule',-2.5,1.35,4.65,5.2,hallcarpet)
cube('Structural cabin deck',(0,5.82,-.08),(5.2,11.8,.15),dark)
# Continuous exterior shell with real openings at the window stations.
window_y=[.65,1.55,2.45,3.35,5.85,6.75,7.65,8.55,9.45,10.35,11.25]
for y0,y1 in [(0,4.65),(5.2,11.65)]:
 cube('Lower cabin wall',(-2.56,(y0+y1)/2,.40),(.14,y1-y0,.8),ivory)
 cube('Upper cabin wall',(-2.51,(y0+y1)/2,2.08),(.16,y1-y0,.54),ivory)
 # Slightly arched ceiling meets the sidewall without a square residential corner.
 v=[];f=[];nx=32
 for yy in [y0,y1]:
  for i in range(nx+1):
   x=-2.56+3.97*i/nx;z=2.56-.38*(abs((x+.55)/2.04)**3);v.append((x,yy,z))
 for i in range(nx):f.append((i,i+1,nx+2+i,nx+1+i))
 o=mesh('CEILING | Curved acoustic headliner',v,f,ivory)
 for p in o.data.polygons:p.use_smooth=True
 cube('Cabin ceiling service reveal',(.99,(y0+y1)/2,2.45),(.035,y1-y0,.06),dark)
 for yy in [y0+.32+i*.75 for i in range(int((y1-y0)/.75))]:
  curve('Headliner expansion seam',[(-2.4,yy,2.30),(-1.8,yy,2.50),(-.5,yy,2.555),(1.32,yy,2.26)],.0025,seam)
for y in [0,4.66,5.18,11.66]:cube('Cabin transverse bulkhead',(-.60,y,1.30),(3.94,.12,2.60),ivory,.014)
for yy,side in [(0,.065),(4.66,-.065),(5.18,.065)]:
 for xx in [-2.0,-1.25,-.5,.25,1.0]:
  curve('Upholstered bulkhead panel seam',[(xx,yy+side,.08),(xx,yy+side,2.42)],.0018,seam)
cube('Hall outboard wall',(2.61,5.83,1.18),(.12,11.8,2.36),ivory)
cube('CEILING | Hall soffit',(1.99,5.83,2.40),(1.25,11.8,.10),ivory)
for yy in [0,11.68]:cube('Hall end bulkhead',(1.98,yy,1.2),(1.25,.12,2.4),wood)
def rounded(w,h,r,n=12):
 pts=[]
 for cx,cz,a in [(w/2-r,h/2-r,0),(-w/2+r,h/2-r,pi/2),(-w/2+r,-h/2+r,pi),(w/2-r,-h/2+r,3*pi/2)]:
  for i in range(n):t=a+i*pi/2/(n-1);pts.append((cx+r*cos(t),cz+r*sin(t)))
 return pts
collection('02 | Oval windows and draperies')
for y in window_y:
 # Filling the wall between the openings leaves a rounded recessed window reveal.
 for sy in [-1,1]:cube('Window station divider',(-2.54,y+sy*.355,1.30),(.13,.19,1.02),ivory,.02)
 outer=rounded(.72,1.04,.17);inner=rounded(.43,.70,.15);v=[]
 for loop,x in [(outer,-2.455),(inner,-2.57),(inner,-2.67)]:v.extend((x,y+u,1.30+z) for u,z in loop)
 n=len(outer);f=[]
 for ring in range(2):
  for i in range(n):f.append((ring*n+i,ring*n+(i+1)%n,(ring+1)*n+(i+1)%n,(ring+1)*n+i))
 o=mesh('Deep oval window surround',v,f,cream)
 for p in o.data.polygons:p.use_smooth=True
 curve('Window inner gasket',[(-2.66,y+u,1.30+z) for u,z in inner],.009,dark,True)
 mesh('Cabin window glass',[(-2.68,y+u,1.30+z) for u,z in inner],[tuple(range(n))],glass)
 cube('Window blind handle',(-2.445,y,1.70),(.04,.10,.016),cream,.006)
 # Small overhead ventilation and reading-light panel.
 cube('Passenger service inset',(-1.85,y,2.48),(.30,.35,.026),cream,.028)
 for u in [-.075,.075]:cyl('Cabin air nozzle',(-1.85+u,y,2.453),.036,.027,trim)
# Opaque panels bridge the small gaps between room sections.
cube('Vestibule curved sidewall',(-2.53,4.93,1.2),(.15,.57,2.4),ivory)
for y in [0,4.15,5.28,11.6]:
 v=[];uv=[];f=[];nx=32;nz=24
 for j in range(nz+1):
  for i in range(nx+1):u=i/nx;q=j/nz;v.append((-2.29+.037*sin(u*pi*16),y+(u-.5)*.34,.65+q*1.62));uv.append((u,q))
 for j in range(nz):
  for i in range(nx):a=j*(nx+1)+i;f.append((a,a+1,a+nx+2,a+nx+1))
 o=mesh('Pleated cream window drape',v,f,ivory,uv)
 for p in o.data.polygons:p.use_smooth=True
collection('03 | Passage and sliding doors')
# Door gaps are geometric openings rather than painted doors.
for a,b in [(0,3.05),(4.10,5.55),(6.60,11.66)]:cube('Suite partition wall',(1.39,(a+b)/2,1.21),(.12,b-a,2.42),ivory)
for a,b,label in [(3.05,4.10,'PRESIDENTIAL OFFICE'),(5.55,6.60,'CONFERENCE ROOM')]:
 mid=(a+b)/2;cube('Door lintel',(1.39,mid,2.27),(.19,1.10,.30),wood,.016)
 for y in [a,b]:cube('Door jamb',(1.39,y,1.05),(.18,.045,2.10),wood,.008)
 cube('Flush threshold',(1.39,mid,.022),(.19,1.04,.016),trim,.003)
 door=cube('Open pocket door | '+label,(1.415,b+.46,1.03),(.07,.92,2.03),wood,.025)
 cube('Pocket door inset handle',(1.465,b+.11,1.04),(.018,.026,.15),trim,.008)
 # Hall-facing sign set in its own shallow panel.
 cube('Cabin room plaque',(1.48,b+.19,1.68),(.018,.29,.10),dark,.008)
 o=text(label,label,(1.493,b+.19,1.674),.019,trim);o.rotation_euler=(pi/2,0,pi/2)
for side in [1.48,2.54]:
 for a,b in ([(0,3.05),(4.10,5.55),(6.60,11.6)] if side==1.48 else [(0,11.6)]):
  cube('Hall walnut lower rail',(side,(a+b)/2,.33),(.025,b-a,.64),wood)
  curve('Continuous hallway handrail',[(side,a+.04,.90),(side,b-.04,.90)],.018,trim)
for yy in [1.2,4.8,8.4,10.9]:
 cube('Hall overhead diffuser',(2.0,yy,2.337),(.68,.18,.014),cream,.04)
 area('Hall warm pool',(2.0,yy,2.30),(2,yy,0),35,.7)
collection('04 | Executive furniture')
def welt(name,center,size,ma=seam):
 x,y,z=center;w,d=size;r=.06;pts=[]
 for cx,cy,a in [(x+w/2-r,y+d/2-r,0),(x-w/2+r,y+d/2-r,pi/2),(x-w/2+r,y-d/2+r,pi),(x+w/2-r,y-d/2+r,3*pi/2)]:
  for i in range(8):t=a+i*pi/14;pts.append((cx+r*cos(t),cy+r*sin(t),z))
 curve(name,pts,.0023,ma,True)
def tufted_back(name,width,z0,height,y):
 nx=32;nz=36;v=[];uv=[];f=[]
 for j in range(nz+1):
  q=j/nz
  for i in range(nx+1):
   u=i/nx;x=(u-.5)*width;z=z0+q*height;puff=.048*sin(pi*u)**.5*sin(pi*q)**.5
   dent=sum(.052*math.exp(-(((u-bu)/.07)**2+((q-bq)/.06)**2)) for bu in [.28,.72] for bq in [.28,.68])
   v.append((x,y-puff+dent,z));uv.append((u,q))
 for j in range(nz):
  for i in range(nx):a=j*(nx+1)+i;f.append((a,a+1,a+nx+2,a+nx+1))
 o=mesh(name,v,f,leather,uv)
 for p in o.data.polygons:p.use_smooth=True
 for u in [.28,.72]:
  for q in [.28,.68]:sphere('Leather covered tuft button',((u-.5)*width,y+.008,z0+q*height),(.018,.009,.018),leather)
def chair(loc,angle=0,executive=False):
 before=set(bpy.data.objects);h=1.5 if executive else 1.16
 cube('Seat pedestal',(0,0,.21),(.36,.40,.36),wood,.035)
 cube('Sculpted leather seat',(0,-.02,.47),(.64,.65,.17),leather,.065);welt('Seat stitched perimeter',(0,-.02,.543),(.59,.60))
 cube('Seat back shell',(0,.255,(.57+h)/2),(.65,.19,h-.57),leather,.065)
 tufted_back('Hand tufted chair back',.60,.59,h-.62,.145)
 if executive:cube('Executive padded headrest',(0,.23,1.56),(.64,.23,.24),leather,.065)
 for x in [-.35,.35]:
  cube('Padded leather arm',(x,-.025,.66),(.13,.67,.15),leather,.047)
  cube('Arm upright',(x,.14,.49),(.10,.18,.28),wood,.025)
  cube('Arm control bezel',(x,-.23,.652),(.07,.10,.012),dark,.006)
  for yy in [-.25,-.215]:cube('Seat adjustment switch',(x,yy,.663),(.045,.019,.008),trim,.003)
 for x in [-.20,.20]:
  belt=cube('Loose seat belt',(x,-.10,.565),(.055,.36,.008),navy,.003);belt.rotation_euler.z=x*.8
  cube('Seat belt buckle',(x,-.26,.576),(.065,.075,.017),trim,.008)
 group(before,loc,angle)
# The desk has an angled return, matching the compact cockpit-like office in the reference.
deskpoly=[(-1.97,.92),(-.45,.92),(.20,1.50),(.20,2.58),(-.47,2.58),(-.47,1.66),(-1.97,1.66)]
def prism(name,pts,z0,z1,ma,bevel=.02):
 n=len(pts);v=[(x,y,z) for z in [z0,z1] for x,y in pts];f=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)];o=mesh(name,v,f,ma)
 # Box-like furniture uses physical planar UVs, preserving wood direction in export.
 layer=o.data.uv_layers.new()
 for poly in o.data.polygons:
  no=poly.normal
  for li in poly.loop_indices:
   co=o.data.vertices[o.data.loops[li].vertex_index].co;layer.data[li].uv=(co.x,co.y) if abs(no.z)>.5 else ((co.x,co.z) if abs(no.y)>.5 else (co.y,co.z))
 b=o.modifiers.new('Rounded cabinet edging','BEVEL');b.width=bevel;b.segments=4;o.modifiers.new('Cabinet normals','WEIGHTED_NORMAL');return o
prism('Angular walnut presidential desktop',deskpoly,.73,.795,wood)
cube('Desk front cabinet',(-.13,2.06,.375),(.59,1.07,.70),wood,.025)
cube('Desk return modesty panel',(-1.16,1.57,.39),(1.55,.09,.67),wood,.015)
cube('Desk return pedestal',(-1.79,1.27,.38),(.34,.60,.69),wood,.02)
curve('Desktop fine brass inlay',[(x,y,.791) for x,y in deskpoly],.0025,trim,True)
for x,y,w,d in [(-1.11,1.29,1.15,.52),(-.12,2.12,.49,.70)]:
 cube('Blotter leather border',(x,y,.807),(w,d,.012),leather,.016);cube('Inset green writing surface',(x,y,.815),(w-.045,d-.065,.008),green,.01)
chair((-1.17,.48,0),pi,True);chair((-1.13,2.87,0),pi-.15,True)
# Low built-in cabinets and the side sofa.
cube('Window-side walnut credenza',(-2.17,2.20,.38),(.53,4.08,.73),wood,.02)
cube('Credenza polished rim',(-2.14,2.20,.77),(.59,4.10,.048),wood,.022)
for i in range(24):cube('Credenza ventilation grille',(-1.876,3.55+i*.015,.23),(.015,.005,.17),dark,.001)
for y in [.50,1.23,1.96,2.69,3.42]:
 cube('Flush cabinet door',(-1.892,y,.42),(.017,.68,.57),wood,.009);cube('Cabinet latch',(-1.876,y+.23,.62),(.018,.06,.018),trim,.004)
def sofa(loc,angle=0):
 before=set(bpy.data.objects)
 cube('Sofa shadow plinth',(0,0,.12),(2.45,.69,.20),dark,.025)
 cube('Sofa upholstered rail',(0,0,.31),(2.53,.81,.31),leather,.065)
 cube('Sofa padded back',(0,.31,.75),(2.53,.20,.79),leather,.065)
 for i in range(3):
  x=(i-1)*.77;cube('Sofa individual seat',(x,-.06,.48),(.755,.66,.18),leather,.065);welt('Sofa cushion seam',(x,-.06,.565),(.70,.60))
  beforepart=set(bpy.data.objects);tufted_back('Tufted sofa back',.74,.55,.52,.18);group(beforepart,(x,0,0))
 for x in [-1.22,1.22]:cube('Sofa rolled end arm',(x,-.03,.63),(.15,.84,.60),leather,.06)
 group(before,loc,angle)
sofa((.89,1.66,0),-pi/2)
collection('05 | Conference suite')
tablepts=rounded(1.32,3.42,.30,16);tablepts=[(x-.58,y+8.62) for x,y in tablepts]
prism('Conference oval walnut tabletop',tablepts,.74,.805,wood,.022)
for y in [7.72,9.52]:cube('Conference pedestal',(-.58,y,.39),(.53,.47,.70),wood,.055)
for y in [7.46,8.60,9.74]:
 chair((-1.70,y,0),pi/2);chair((.58,y,0),-pi/2)
chair((-.58,6.32,0),pi);chair((-.58,10.91,0),0)
for y in [7.44,8.58,9.72]:
 for x in [-1.02,-.14]:cube('Conference writing pad',(x,y,.813),(.36,.52,.012),green,.015)
cube('Conference credenza',(-2.19,8.48,.37),(.48,5.6,.70),wood,.02)
for y in [6.03,6.75,7.47,8.19,8.91,9.63,10.35,11.07]:cube('Conference cabinet front',(-1.938,y,.37),(.015,.68,.58),wood,.007)
cube('Monitor bulkhead bezel',(-.57,11.57,1.55),(1.32,.035,.77),dark,.028)
screen=mat('Briefing display standby',(.007,.017,.027),.22);screen.node_tree.nodes.get('Principled BSDF').inputs['Emission Color'].default_value=(.012,.038,.065,1);screen.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength'].default_value=.5
cube('Briefing display',(-.57,11.545,1.55),(1.20,.01,.67),screen,.008)
text('Briefing display title','IN FLIGHT BRIEFING',(-.57,11.529,1.63),.063,paper)
text('Briefing display subtitle','PRESIDENTIAL AIRLIFT',(-.57,11.528,1.48),.027,trim)
collection('06 | Desk objects and cabin details')
def phone(loc,angle=0):
 before=set(bpy.data.objects)
 cube('Telephone console',(0,0,.04),(.25,.22,.075),black,.019)
 p=cube('Phone inclined display',(0,.066,.11),(.18,.075,.013),dark,.006);p.rotation_euler.x=.65
 cube('Handset cradle',(-.097,0,.10),(.07,.21,.055),black,.02)
 for y in [-.083,.083]:sphere('Telephone handset end',(-.097,y,.117),(.048,.045,.022),black)
 for i in range(3):
  for j in range(4):cube('Telephone keypad',(-.025+i*.043,-.079+j*.034,.085),(.029,.023,.009),cream,.004)
 pts=[(-.18+.017*cos(i*.8),-.06+i*.002,.05+.017*sin(i*.8)) for i in range(100)];curve('Coiled handset cable',pts,.003,black)
 group(before,loc,angle)
phone((-1.77,1.27,.80),-.1);phone((-.57,8.62,.812),0)
def lamp(loc):
 before=set(bpy.data.objects);cyl('Lamp weighted foot',(0,0,.03),.11,.05,trim);cyl('Lamp ceramic body',(0,0,.145),.077,.21,cream);cyl('Lamp neck',(0,0,.29),.018,.14,trim)
 vv=[];ff=[]
 for z,r in [(.29,.20),(.55,.115),(.55,.108),(.29,.193)]:
  for i in range(64):a=2*pi*i/64;vv.append((r*cos(a),r*sin(a),z))
 for j in range(3):
  for i in range(64):ff.append((j*64+i,j*64+(i+1)%64,(j+1)*64+(i+1)%64,(j+1)*64+i))
 shade=mat('Linen lamp shade',(.84,.79,.64),.85);bs=shade.node_tree.nodes.get('Principled BSDF');bs.inputs['Emission Color'].default_value=(1,.72,.37,1);bs.inputs['Emission Strength'].default_value=.12
 mesh('Pleated linen lampshade',vv,ff,shade);cyl('Lamp finial',(0,0,.57),.017,.04,trim)
 for i in range(48):a=2*pi*i/48;curve('Linen shade seam',[(.201*cos(a),.201*sin(a),.29),(.116*cos(a),.116*sin(a),.55)],.0007,seam)
 area('Lamp warm bounce',(0,0,.40),(0,-1,.60),12,.22,(1,.67,.34))
 group(before,loc)
lamp((-2.10,.29,.80));lamp((-2.1,3.81,.80));lamp((-2.12,5.63,.73))
def paperstack(x,y,z):
 cube('Briefing folio',(x,y,z),(.26,.34,.026),navy,.006)
 for i in range(3):
  o=cube('Briefing paper',(x+.002*i,y+.002*i,z+.015+i*.002),(.237,.31,.0015),paper,.0003);o.rotation_euler.z=.02*i
 for i in range(8):cube('Printed document line',(x,y-.08+i*.019,z+.023),(.16 if i else .19,.0013,.0004),dark,0)
paperstack(-.12,2.16,.831);paperstack(-1.02,7.47,.833);paperstack(-.15,9.73,.833)
for x,y,z in [(-.14,1.82,.837),(-1.04,7.11,.84),(-.11,9.37,.84)]:
 o=cyl('Brass writing pen',(x,y,z),.004,.14,trim,12);o.rotation_euler.y=pi/2
for x,y in [(-.12,7.4),(-1.00,9.7)]:
 cyl('Porcelain cup saucer',(x,y+.25,.822),.073,.01,cream)
 cyl('Coffee cup',(x,y+.25,.87),.037,.087,cream);cyl('Coffee surface',(x,y+.25,.915),.032,.002,wood)
 curve('Cup handle',[(x+.035+.026*cos(a),y+.25,.872+.031*sin(a)) for a in [2*pi*i/32 for i in range(33)]],.006,cream)
# A small presidential emblem recalls the blind badge in the supplied photo.
sealpath=P/'textures'/'presidential-seal.png'
if sealpath.exists():
 badge=mat('Presidential seal emblem',(.8,.8,.8),.66);nt=badge.node_tree.nodes.new('ShaderNodeTexImage');nt.image=bpy.data.images.load(str(sealpath));badge.node_tree.links.new(nt.outputs['Color'],badge.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
 pts=[(-2.445,.65,1.58)]+[(-2.445,.65+.083*cos(2*pi*i/64),1.58+.083*sin(2*pi*i/64)) for i in range(64)];uv=[(.5,.5)]+[(.5+.5*cos(2*pi*i/64),.5+.5*sin(2*pi*i/64)) for i in range(64)];mesh('Presidential blind badge',pts,[(0,i+1,(i+1)%64+1) for i in range(64)],badge,uv)
collection('07 | Daylight and cabin illumination')
world=bpy.data.worlds.new('High altitude daylight');world.use_nodes=True;world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.56,.72,.88,1);world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.5;S.world=world
cloud=image_mat('Cloud deck outside windows','cloud-deck',1,0);bs=cloud.node_tree.nodes.get('Principled BSDF');tex=next(n for n in cloud.node_tree.nodes if n.type=='TEX_IMAGE');cloud.node_tree.links.new(tex.outputs['Color'],bs.inputs['Emission Color']);bs.inputs['Emission Strength'].default_value=.5
plane('EXTERIOR | Distant cloud deck',[(-4,-4,-1),(-4,16,-1),(-4,16,5),(-4,-4,5)],[(0,0),(1,0),(1,1),(0,1)],cloud)
for y in window_y:area('Window daylight',(-2.73,y,1.37),(-.4,y+.1,.85),38,.48,(.77,.87,1),'RECTANGLE',.72)
for y in [1.1,3.3,6.5,8.5,10.5]:
 area('Soft ceiling bounce',(-.60,y,2.43),(-.60,y,0),48,2.2,(1,.86,.68),'RECTANGLE',1.0)
 cube('Recessed ceiling luminous strip',(-.50,y,2.52),(1.15,.055,.01),cream,.015)
collection('08 | Cameras and future game markers')
def camera(name,loc,target,lens):
 d=bpy.data.cameras.new(name);d.lens=lens;d.clip_start=.03;d.clip_end=150;o=bpy.data.objects.new(name,d);C.objects.link(o);o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();return o
hero=camera('01 | Presidential office',(.98,4.32,1.58),(-.91,1.70,1.05),21)
conference=camera('02 | Conference room',(.98,5.52,1.64),(-.64,8.65,1.06),22)
hall=camera('03 | Connecting hallway',(2.00,1.00,1.62),(1.92,8.0,1.40),24)
overview=camera('04 | Three-room cutaway',(12,17,15),(-.15,5.8,0),43)
for name,loc in [('PLAYER_START_OFFICE',(.60,3.62,.05)),('PLAYER_START_HALL',(1.95,3.6,.05)),('PLAYER_START_CONFERENCE',(.80,6.1,.05)),('INTERACT_PRESIDENT_DESK',(.35,2.0,.90)),('INTERACT_BRIEFING_TABLE',(-.58,8.62,.85))]:
 o=bpy.data.objects.new(name,None);C.objects.link(o);o.location=loc;o.empty_display_type='ARROWS';o.empty_display_size=.18
S.camera=hero;S.render.engine='CYCLES';S.cycles.device='CPU';S.cycles.samples=56;S.cycles.use_denoising=True;S.cycles.max_bounces=7;S.cycles.diffuse_bounces=3;S.cycles.glossy_bounces=3;S.cycles.transmission_bounces=6
S.render.resolution_x=1500;S.render.resolution_y=1000;S.render.resolution_percentage=100;S.render.image_settings.file_format='PNG';S.render.image_settings.color_mode='RGB'
S.view_settings.view_transform='AgX';S.view_settings.look='AgX - Medium High Contrast';S.view_settings.exposure=.3
S['Project']='Air Force One | compact presidential suite';S['Accuracy']='Artistic, public-reference reconstruction. Office based on supplied photograph and ABC News tour; hallway and conference layout adapted for a compact game level.'
S['Units']='metres';S['Game integration']='Connected door openings, named rooms, player starts and interaction markers. Gameplay and collision import not yet implemented.'
for sc in bpy.data.screens:
 for a in sc.areas:
  if a.type=='VIEW_3D':a.spaces.active.region_3d.view_perspective='CAMERA';a.spaces.active.shading.type='MATERIAL'
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(P/'Air_Force_One.blend'))
print('BUILD_READY',len(S.objects),'objects',flush=True)
if '--preview' in sys.argv:
 S.render.resolution_x=1050;S.render.resolution_y=700;S.cycles.samples=24;S.render.filepath=str(P/'renders'/'Office_preview.png');bpy.ops.render.render(write_still=True)
