import bpy, math, random, os, sys
from mathutils import Vector
from pathlib import Path
from math import sin, cos, pi
ROOT=Path(__file__).resolve().parent
random.seed(17)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for x in list(bpy.data.materials):bpy.data.materials.remove(x)
scene=bpy.context.scene
scene.unit_settings.system='METRIC'

def coll(name):
 print('BUILDING',name,flush=True);c=bpy.data.collections.new(name);scene.collection.children.link(c);return c
C=coll('01 | Elliptical architecture')
def move(o):
 for c in list(o.users_collection):c.objects.unlink(o)
 C.objects.link(o);return o
def mat(name,col,rough=.5,metal=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*col,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*col,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal;return m
def noise_mat(name,c1,c2,scale=8,rough=.5,bump=.03,stretch=None):
 m=mat(name,c1,rough);n=m.node_tree.nodes;l=m.node_tree.links;p=n.get('Principled BSDF');tex=n.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=scale;tex.inputs['Detail'].default_value=3;tex.inputs['Roughness'].default_value=.75
 if stretch:
  tc=n.new('ShaderNodeTexCoord');v=n.new('ShaderNodeVectorMath');v.operation='MULTIPLY';v.inputs[1].default_value=stretch;l.new(tc.outputs['Generated'],v.inputs[0]);l.new(v.outputs[0],tex.inputs['Vector'])
 ramp=n.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.2;ramp.color_ramp.elements[0].color=(*c1,1);ramp.color_ramp.elements[1].position=.8;ramp.color_ramp.elements[1].color=(*c2,1);l.new(tex.outputs['Fac'],ramp.inputs[0]);l.new(ramp.outputs[0],p.inputs['Base Color'])
 b=n.new('ShaderNodeBump');b.inputs['Strength'].default_value=.3;b.inputs['Distance'].default_value=bump;l.new(tex.outputs['Fac'],b.inputs['Height']);l.new(b.outputs[0],p.inputs['Normal']);return m
def image_mat(name,file,rough=.65,bump=0):
 m=mat(name,(.5,.5,.5),rough);n=m.node_tree.nodes;l=m.node_tree.links;p=n.get('Principled BSDF');t=n.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(ROOT/'textures'/file));l.new(t.outputs['Color'],p.inputs['Base Color'])
 if bump:
  no=n.new('ShaderNodeTexNoise');no.inputs['Scale'].default_value=700;b=n.new('ShaderNodeBump');b.inputs['Strength'].default_value=.3;b.inputs['Distance'].default_value=bump;l.new(no.outputs[0],b.inputs['Height']);l.new(b.outputs[0],p.inputs['Normal'])
 return m
ivory=mat('Warm ivory painted joinery',(.78,.765,.68),.32)
white=mat('Window enamel',(.87,.865,.80),.25)
plaster=noise_mat('Lime plaster',(.76,.74,.66),(.85,.83,.76),85,.9,.004)
wall=noise_mat('Handmade wheat wallpaper',(.65,.59,.47),(.71,.66,.55),95,.87,.001)
stripe=noise_mat('Wallpaper pale satin stripe',(.75,.70,.60),(.80,.76,.66),100,.79,.001)
wood=noise_mat('Resolute | aged English oak',(.045,.011,.004),(.17,.058,.018),5,.29,.0008,(1,28,3))
woodlight=noise_mat('Carved oak highlights',(.075,.024,.008),(.20,.073,.025),6,.36,.001,(1,22,2))
wooddark=noise_mat('Oak recessed panels',(.031,.009,.003),(.10,.031,.009),6,.43,.001,(2,24,1))
mahogany=noise_mat('Antique mahogany',(.06,.015,.007),(.22,.065,.021),4,.29,.003,(1,18,4))
gold=mat('Aged gilt bronze',(.48,.29,.080),.27,.78)
brass=mat('Burnished brass',(.42,.30,.12),.24,.75)
black=mat('Ebony',(.012,.015,.016),.33)
leather=noise_mat('Chocolate leather',(.07,.039,.022),(.105,.067,.039),165,.42,.002)
fabric=noise_mat('Oatmeal sofa | linen weave',(.52,.47,.37),(.68,.62,.51),230,.87,.0009)
fabric.node_tree.nodes.get('Principled BSDF').inputs['Sheen Weight'].default_value=.28
piping=mat('Linen welting',(.57,.52,.40),.87)
red=noise_mat('Crimson silk damask',(.095,.006,.005),(.24,.022,.012),22,.62,.0003,(7,7,1))
red.node_tree.nodes.get('Principled BSDF').inputs['Sheen Weight'].default_value=.10
red.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.84
rugmat=image_mat('Wool carpet | woven historical quotations','rug.png',.97,.014)
usmat=image_mat('United States flag | woven silk','us_flag.png',.66,.002)
presmat=image_mat('Presidential standard | woven silk','presidential_flag.png',.67,.002)
avemat=image_mat('Hassam | The Avenue in the Rain, 1917','avenue.jpg',.72,.001)
libmat=image_mat('Rockwell | Working on the Statue of Liberty','liberty.jpg',.72,.001)

def cube(name,loc,dims,ma,bevel=0):
 x,y,z=[d/2 for d in dims];me=bpy.data.meshes.new(name);me.from_pydata([(-x,-y,-z),(-x,-y,z),(-x,y,-z),(-x,y,z),(x,-y,-z),(x,-y,z),(x,y,-z),(x,y,z)],[],[(0,4,6,2),(1,3,7,5),(0,1,5,4),(2,6,7,3),(0,2,3,1),(4,5,7,6)]);me.update();o=bpy.data.objects.new(name,me);C.objects.link(o);o.location=loc
 if ma:o.data.materials.append(ma)
 if bevel:
  b=o.modifiers.new('Softened craftsmanship edges','BEVEL');b.width=bevel;b.segments=3
  o.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL')
 return o
def sphere(name,loc,scale,ma):
 import bmesh
 me=bpy.data.meshes.new(name);bm=bmesh.new();bmesh.ops.create_uvsphere(bm,u_segments=20,v_segments=12,radius=1);bm.to_mesh(me);bm.free();o=bpy.data.objects.new(name,me);C.objects.link(o);o.location=loc;o.scale=scale;o.data.materials.append(ma)
 for p in o.data.polygons:p.use_smooth=True
 return o
def cyl(name,loc,r,depth,ma,verts=40):
 v=[(r*cos(2*pi*i/verts),r*sin(2*pi*i/verts),z) for z in [-depth/2,depth/2] for i in range(verts)];f=[tuple(range(verts-1,-1,-1)),tuple(range(verts,2*verts))]+[(i,(i+1)%verts,(i+1)%verts+verts,i+verts) for i in range(verts)];o=mesh(name,v,f,ma);o.location=loc;b=o.modifiers.new('Edge glint','BEVEL');b.width=.003;b.segments=2;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL');return o
def curve(name,pts,r,ma,closed=False):
 cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.resolution_u=2;cu.bevel_depth=r;cu.bevel_resolution=3;s=cu.splines.new('POLY');s.points.add(len(pts)-1)
 for p,co in zip(s.points,pts):p.co=(*co,1)
 s.use_cyclic_u=closed;o=bpy.data.objects.new(name,cu);C.objects.link(o);o.data.materials.append(ma);return o
def mesh(name,v,f,ma,uv=None):
 me=bpy.data.meshes.new(name);me.from_pydata(v,[],f);me.update();o=bpy.data.objects.new(name,me);C.objects.link(o);o.data.materials.append(ma)
 if uv:
  la=me.uv_layers.new()
  for p in me.polygons:
   for li in p.loop_indices:la.data[li].uv=uv[me.loops[li].vertex_index]
 return o
def plane_front(name,x,y,z,w,h,ma):
 return mesh(name,[(x-w/2,y,z-h/2),(x+w/2,y,z-h/2),(x+w/2,y,z+h/2),(x-w/2,y,z+h/2)],[(0,1,2,3)],ma,[(0,0),(1,0),(1,1),(0,1)])
def ring(name,a,b,z,r,ma):return curve(name,[(a*cos(t*2*pi/384),b*sin(t*2*pi/384),z) for t in range(384)],r,ma,True)
def frame(name,x,y,z,w,h,ma=gold,thick=.04):
 for dx in [-w/2,w/2]:cube(name+' vertical',(x+dx,y,z),(thick,.06,h+thick),ma,.007)
 for dz in [-h/2,h/2]:cube(name+' horizontal',(x,y,z+dz),(w,.06,thick),ma,.007)
def group_transform(before,loc,angle):
 from mathutils import Matrix
 bpy.context.view_layer.update()
 R=Matrix.Rotation(angle,4,'Z');T=Matrix.Translation(Vector(loc))
 for o in set(bpy.data.objects)-before:o.matrix_world=T@R@o.matrix_world

# Room shell; curved plaster sections leave actual openings for three tall sash windows.
A=4.45;B=5.46;H=5.45
windows=[(pi/2-.43,.17),(pi/2,.17),(pi/2+.43,.17)]
def iswindow(t):return any(abs(t-c)<w for c,w in windows)
N=768
for i in range(N):
 t=(i+.5)*2*pi/N;t0=i*2*pi/N;t1=(i+1)*2*pi/N
 ranges=[(0,.36),(4.54,H)] if iswindow(t) else [(0,H)]
 for z0,z1 in ranges:
  mesh('Curved plaster wall',[(A*cos(t0),B*sin(t0),z0),(A*cos(t1),B*sin(t1),z0),(A*cos(t1),B*sin(t1),z1),(A*cos(t0),B*sin(t0),z1)],[(0,1,2,3)],wall)
 if not iswindow(t) and i%4<2:
  mesh('Vertical hand-papered stripe',[(A*.999*cos(t0),B*.999*sin(t0),1.03),(A*.999*cos(t1),B*.999*sin(t1),1.03),(A*.999*cos(t1),B*.999*sin(t1),5.1),(A*.999*cos(t0),B*.999*sin(t0),5.1)],[(0,1,2,3)],stripe)
# Floor parquet individual planks in alternating basket runs, clipped at ellipse.
floorM=[]
for i in range(7):
 k=.85+i*.05;floorM.append(noise_mat('Oak parquet tone '+str(i),(.16*k,.068*k,.021*k),(.37*k,.19*k,.065*k),5,.36,.002,(1,27,3)))
cube('Subfloor',(0,0,-.055),(9,11,.1),wooddark)
for j in range(-27,28):
 y=j*.20
 for i in range(-5,5):
  x=i*1.04+(j%3)*.345
  if (x/A)**2+(y/B)**2<1.02:cube('Quarter sawn parquet',(x,y,-.005),(1.035,.196,.026),random.choice(floorM),.0015)
for z,r in [(.06,.035),(.16,.024),(.94,.022),(1.015,.027),(1.065,.018)]:ring('Continuous elliptical dado moulding',A-.025,B-.025,z,r,ivory)
# Dado panel rails follow the curve and are individually mitred.
for j in range(76):
 t=2*pi*(j+.5)/76
 if iswindow(t):continue
 x=(A-.04)*cos(t);y=(B-.04)*sin(t);ang=math.atan2(B*cos(t),-A*sin(t))
 o=cube('Ivory wainscot panel',(x,y,.53),(.39,.06,.70),ivory,.006);o.rotation_euler.z=ang
 for zz in [.22,.84]:
  o=cube('Raised panel bead',(x-.025*cos(t),y-.025*sin(t),zz),(.37,.035,.018),white,.004);o.rotation_euler.z=ang
 for u in [-.18,.18]:
  o=cube('Wainscot stile',(x+u*cos(ang)-.03*cos(t),y+u*sin(ang)-.03*sin(t),.53),(.017,.035,.63),white,.004);o.rotation_euler.z=ang
profile=[(0,5.00),(.045,5.00),(.045,5.055),(.073,5.065),(.085,5.13),(.12,5.19),(.19,5.24),(.27,5.26),(.27,5.31),(.33,5.31),(.33,5.37),(.39,5.37),(.39,5.45),(0,5.45)]
vv=[];ff=[]
for off,z in profile:
 for i in range(384):t=2*pi*i/384;vv.append(((A-off)*cos(t),(B-off)*sin(t),z))
for j in range(len(profile)-1):
 for i in range(384):ff.append((j*384+i,j*384+(i+1)%384,(j+1)*384+(i+1)%384,(j+1)*384+i))
mesh('Classical continuous plaster cornice profile',vv,ff,ivory)
ring('Cornice fine lower bead',A-.05,B-.05,5.056,.013,ivory)
cube('Ceiling',(0,0,5.50),(9.4,11.6,.1),plaster)

# Windows, sculpted pleats, scalloped swags and gold fringe.
def curtain(name,x,y,width,z0,z1,phase=0):
 nx=72;nz=36;v=[];uv=[]
 for j in range(nz+1):
  f=j/nz;z=z0+(z1-z0)*f
  for i in range(nx+1):
   u=i/nx;xx=x+(u-.5)*width*(1+.12*(1-f)**3);dep=.068*sin(u*pi*18+phase)+.020*sin(u*pi*36+.5)
   v.append((xx,y+dep,z+.025*sin(u*pi*18)*(1-f)**8));uv.append((u,f))
 faces=[(j*(nx+1)+i,j*(nx+1)+i+1,(j+1)*(nx+1)+i+1,(j+1)*(nx+1)+i) for j in range(nz) for i in range(nx)]
 o=mesh(name,v,faces,red,uv)
 for p in o.data.polygons:p.use_smooth=True
 s=o.modifiers.new('Silk thickness','SOLIDIFY');s.thickness=.006
 return o
for idx,(t,w) in enumerate(windows):
 before=set(bpy.data.objects);cx=A*cos(t);cy=B*sin(t);ang=math.atan2(B*cos(t),-A*sin(t))-pi
 # Local front is -Y and maps to room interior.
 width=1.45;height=4.16;zc=2.45
 for x in [-width/2,width/2]:
  cube('Deep window reveal',(x,.06,zc),(.15,.3,height+.14),ivory,.013)
  cube('Architrave outer casing',(x,-.12,zc),(.11,.08,height+.29),white,.011)
 for z in [.37,4.54]:cube('Window lintel and sill',(0,-.06,z),(width+.23,.30,.10),white,.01)
 for x in [-width/2+.07,0,width/2-.07]:cube('Sash vertical muntin',(x,.045,zc),(.034,.055,4.09),white,.004)
 for j in range(7):cube('Sash crossbar',(0,.025,.43+j*.675),(width-.09,.05,.028 if j!=3 else .055),white,.003)
 # Ultra-clear glazing, thin geometry to preserve visible garden.
 glass=mat('Historic clear window glass '+str(idx),(.96,.98,1),.03);p=glass.node_tree.nodes.get('Principled BSDF');p.inputs['Transmission Weight'].default_value=1;p.inputs['IOR'].default_value=1.45
 gn=glass.node_tree.nodes;gl=glass.node_tree.links;lp=gn.new('ShaderNodeLightPath');tr=gn.new('ShaderNodeBsdfTransparent');mx=gn.new('ShaderNodeMixShader');gl.new(lp.outputs['Is Shadow Ray'],mx.inputs[0]);gl.new(p.outputs[0],mx.inputs[1]);gl.new(tr.outputs[0],mx.inputs[2]);gl.new(mx.outputs[0],gn.get('Material Output').inputs['Surface'])
 cube('Window glazing',(0,.075,zc),(width-.13,.008,4.07),glass)
 for side in [-1,1]:
  curtain('Full length crimson pleated drape',side*.93,-.22,.54,.10,4.72,idx*.7)
  curve('Gold vertical curtain braid',[(side*(.665+.022*sin(j/10)),-.31,.12+j*4.6/90) for j in range(91)],.008,gold)
 # Dipped valance gathered across the head.
 v=[];f=[];nx=100;ny=16
 for j in range(ny+1):
  q=j/ny
  for i in range(nx+1):
   u=i/nx;dip=.34*sin(pi*u)**.65
   v.append(((u-.5)*2.27,-.25-.012*sin(q*7*pi)-.008*cos(u*pi*16),4.78-q*(.12+dip)))
 for j in range(ny):
  for i in range(nx):k=j*(nx+1)+i;f.append((k,k+1,k+nx+2,k+nx+1))
 o=mesh('Draped scalloped silk pelmet',v,f,red)
 for p in o.data.polygons:p.use_smooth=True
 curve('Valance gold edge',[((i/100-.5)*2.27,-.29,4.66-.34*sin(pi*i/100)**.65) for i in range(101)],.011,gold)
 for i in range(70):
  u=i/69;z=4.66-.34*sin(pi*u)**.65;curve('Valance fine bullion fringe',[((u-.5)*2.27,-.29,z),((u-.5)*2.27,-.29,z-.037)],.003,brass)
 group_transform(before,(cx,cy,0),ang)

# Carpet ellipse with continuous custom UV layout.
C=coll('02 | Carpet and seating')
v=[(0,0,.028)]+[(3.72*cos(2*pi*i/256),4.46*sin(2*pi*i/256),.028) for i in range(256)]
uv=[(.5,.5)]+[(.5+.5*cos(2*pi*i/256),.5+.5*sin(2*pi*i/256)) for i in range(256)]
o=mesh('Oval hand-tufted wool rug',v,[(0,i+1,(i+1)%256+1) for i in range(256)],rugmat,uv);s=o.modifiers.new('Woven carpet thickness','SOLIDIFY');s.thickness=.018
ring('Carpet bound edge',3.72,4.46,.03,.012,piping)

def cushion(name,loc,dim,ma,bev=.09):
 o=cube(name,loc,dim,ma,bev)
 for p in o.data.polygons:p.use_smooth=True
 return o
def welt(name,x,y,z,w,d,r,ma):
 pts=[]
 for cx,cy,start in [(x+w/2-r,y+d/2-r,0),(x-w/2+r,y+d/2-r,pi/2),(x-w/2+r,y-d/2+r,pi),(x+w/2-r,y-d/2+r,3*pi/2)]:
  for i in range(12):
   a=start+i/11*pi/2;pts.append((cx+r*cos(a),cy+r*sin(a),z))
 return curve(name,pts,.004,ma,True)
def sofa(x,y,angle):
 before=set(bpy.data.objects)
 for xx in [-1.05,1.05]:
  for yy in [-.34,.34]:cube('Sofa recessed walnut foot',(xx,yy,.13),(.10,.12,.21),mahogany,.013)
 cushion('Upholstered sofa lower rail',(0,0,.33),(2.55,.92,.34),fabric,.07)
 cushion('Sofa upholstered back',(0,.37,.75),(2.51,.24,.78),fabric,.085)
 for i in range(3):
  xx=(i-1)*.73;cushion('Individual seat cushion',(xx,-.045,.53),(.715,.72,.22),fabric,.08);welt('Seat cushion piping',xx,-.045,.615,.68,.68,.095,piping)
  o=cushion('Soft back cushion',(xx,.245,.83),(.72,.22,.60),fabric,.095);o.rotation_euler.x=.10
 for xx in [-1.19,1.19]:
  cushion('Rounded sofa arm',(xx,-.025,.64),(.24,.91,.60),fabric,.10)
  curve('Arm seam',[(xx,-.47,.72),(xx,-.44,.84),(xx,.35,.84)],.004,piping)
 # striped small pillows use a thread-direction shader.
 pillow=mat('Muted red blue striped accent textile '+str(x),(.4,.3,.2),.9);n=pillow.node_tree.nodes;l=pillow.node_tree.links;tc=n.new('ShaderNodeTexCoord');wave=n.new('ShaderNodeTexWave');wave.wave_type='BANDS';wave.bands_direction='X';wave.inputs['Scale'].default_value=5;l.new(tc.outputs['Generated'],wave.inputs['Vector']);r=n.new('ShaderNodeValToRGB');r.color_ramp.interpolation='CONSTANT';r.color_ramp.elements[0].color=(.18,.30,.37,1);r.color_ramp.elements[1].color=(.63,.47,.27,1);r.color_ramp.elements.new(.32).color=(.44,.16,.09,1);r.color_ramp.elements.new(.55).color=(.72,.64,.43,1);l.new(wave.outputs[0],r.inputs[0]);l.new(r.outputs[0],n.get('Principled BSDF').inputs['Base Color'])
 for xx in [-.89,.89]:
  o=cushion('Striped silk throw pillow',(xx,.08,.86),(.44,.19,.43),pillow,.09);o.rotation_euler=(.15,.22 if xx>0 else -.18,.06)
 group_transform(before,(x,y,0),angle)
sofa(-2.23,-.65,pi/2);sofa(2.23,-.65,-pi/2)

# Mica coffee table: a recessed plinth and a softly reflective mineral top.
mica=noise_mat('Mica clad coffee table',(.24,.17,.10),(.44,.34,.23),38,.34,.004)
cube('Coffee table shadow plinth',(0,-1.05,.15),(1.20,1.20,.25),wooddark,.035)
cube('Mica coffee table body',(0,-1.05,.31),(1.43,1.38,.29),mica,.022)
cube('Mica table top',(0,-1.05,.478),(1.48,1.43,.055),mica,.018)
porcelain=mat('White porcelain',(.84,.85,.78),.20)
def lathe(name,profile,loc,ma,segments=64):
 v=[];f=[]
 for r,z in profile:
  for i in range(segments):a=2*pi*i/segments;v.append((loc[0]+r*cos(a),loc[1]+r*sin(a),loc[2]+z))
 for j in range(len(profile)-1):
  for i in range(segments):a=j*segments+i;b=j*segments+(i+1)%segments;f.append((a,b,b+segments,a+segments))
 o=mesh(name,v,f,ma)
 for p in o.data.polygons:p.use_smooth=True
 return o
lathe('White ceramic fruit bowl',[(0,0),(.12,0),(.17,.03),(.24,.13),(.25,.15),(.238,.15),(.224,.13),(.15,.035),(0,.025)],(0,-1.03,.51),porcelain)
apple=noise_mat('Green apples',(.20,.30,.015),(.47,.52,.048),28,.26,.001)
for i in range(7):
 a=i*2.4;r=.12 if i else 0;x=r*cos(a);y=-1.03+r*sin(a);z=.64+(i%3)*.015;sphere('Fresh apple',(x,y,z),(.066,.064,.063),apple);curve('Apple stem',[(x,y,z+.054),(x+.007,y,z+.081)],.003,wooddark)
paper=mat('Uncoated cream stationery',(.87,.865,.80),.85)
cube('Book on coffee table',(.38,-1.30,.53),(.28,.36,.05),leather,.008);cube('Book pages',(.38,-1.30,.538),(.269,.35,.033),paper,.002)

# Resolute desk, built from a structural carcass, mouldings, relief, and fine carving.
C=coll('03 | Resolute desk and work objects')
D=2.54
cube('Resolute oak desktop',(0,D,.855),(1.95,1.17,.09),wood,.022)
cube('Desk top upper edge',(0,D,.895),(1.97,1.18,.025),woodlight,.008)
cube('Desk apron',(0,D,.755),(1.86,1.10,.17),wood,.012)
cube('Carved front fascia',(0,D-.551,.741),(1.88,.035,.155),wooddark,.006)
for xx in [-.68,.68]:
 cube('Desk pedestal',(xx,D,.385),(.48,1.04,.66),wood,.014)
 cube('Pedestal stepped plinth',(xx,D,.095),(.54,1.10,.15),wood,.016)
 cube('Pedestal plinth bead',(xx,D,.175),(.51,1.065,.035),woodlight,.006)
cube('Central historic modesty panel',(0,D-.505,.408),(.86,.075,.57),wooddark,.008)
for xx in [-.865,-.445,.445,.865]:
 cube('Carved pilaster stile',(xx,D-.577,.425),(.072,.082,.59),woodlight,.006)
 for z in [.15,.20,.64,.69]:cube('Pilaster cap',(xx,D-.585,z),(.099,.093,.027),wood,.006)
 for k in range(3):curve('Pilaster fluting',[(xx+(k-1)*.018,D-.625,z) for z in [.24,.58]],.0035,wooddark)
for xx in [-.66,.66,0]:
 wid=.30 if xx else .74
 frame('Raised oak panel moulding',xx,D-.574,.424,wid,.43,woodlight,.025)
 curve('Arched panel crown',[(xx+wid*.5*cos(a),D-.603,.56+.075*sin(a)) for a in [i*pi/32 for i in range(33)]],.012,woodlight)
# Repeating acanthus curls across frieze and panels.
for i in range(22):
 x=-.85+i*.081
 for sgn in [-1,1]:
  pts=[]
  for j in range(34):
   t=j/33*2*pi;r=.033*(1-j/40);pts.append((x+sgn*r*cos(t),D-.584,.744+r*sin(t)))
  curve('Hand-carved oak frieze scroll',pts,.006,woodlight)
for xx in [-.66,.66]:
 for s in [-1,1]:
  for j in range(8):
   z=.24+j*.044;o=sphere('Raised acanthus leaf',(xx+s*.122,D-.597,z),(.015,.010,.032),woodlight);o.rotation_euler.y=s*.6
for x in [i*.037-.90 for i in range(50)]:sphere('Beaded desk edging',(x,D-.596,.831),(.010,.008,.010),woodlight)
# Carved eagle: layered feathers, spread wings, shield, tail and surrounding wreath.
for s in [-1,1]:
 for i in range(11):
  x=s*(.055+i*.019);z=.45+.082*sin(i/11*pi)
  o=sphere('Eagle feather relief',(x,D-.620,z),(.019,.012,.096-i*.0025),woodlight);o.rotation_euler.y=s*(.65+i*.035)
 for i in range(14):
  a=-pi/2+i/13*pi;x=s*.255*cos(a);z=.418+.224*sin(a);o=sphere('Oak laurel wreath',(x,D-.616,z),(.015,.01,.030),woodlight);o.rotation_euler.y=s*(a+.45)
sphere('Eagle body',(0,D-.64,.432),(.051,.022,.085),woodlight)
sphere('Eagle head',(-.018,D-.648,.537),(.026,.023,.031),woodlight)
curve('Eagle beak',[(-.024,D-.665,.542),(-.048,D-.664,.54),(-.034,D-.664,.530)],.005,woodlight)
for i in range(5):
 o=sphere('Eagle tail',(i*.014-.028,D-.639,.342),(.012,.012,.047),woodlight);o.rotation_euler.y=(i-2)*.13
cube('Eagle shield',(0,D-.666,.431),(.080,.016,.090),wood,.016)
for x in [-.026,-.013,0,.013,.026]:curve('Shield heraldic stripes',[(x,D-.678,.393),(x,D-.678,.45)],.003,woodlight)
curve('Eagle banner',[(-.17,D-.642,.588),(-.10,D-.650,.605),(0,D-.65,.590),(.10,D-.65,.605),(.17,D-.64,.588)],.013,woodlight)
# Photographic oak relief surface over the structural front, preserving carved detail
# beyond the scale practical for hand-authored geometry. UVs select only the desk face.
for ob in list(C.objects):
 if any(k in ob.name for k in ['Eagle','eagle','Oak laurel','Shield heraldic','Carved pilaster','Pilaster','Hand-carved oak frieze','Raised acanthus','Beaded desk','Raised oak panel','Arched panel']):ob.hide_render=True;ob.hide_set(True)
relief=image_mat('Resolute oak | photographic carving relief','../references/desk-reference.jpg',.46)
n=relief.node_tree.nodes;l=relief.node_tree.links;t=next(n for n in n if n.type=='TEX_IMAGE');bu=n.new('ShaderNodeBump');bu.inputs['Strength'].default_value=.25;bu.inputs['Distance'].default_value=.006;l.new(t.outputs['Color'],bu.inputs['Height']);l.new(bu.outputs[0],n.get('Principled BSDF').inputs['Normal'])
hs=n.new('ShaderNodeHueSaturation');hs.inputs['Saturation'].default_value=.67;hs.inputs['Value'].default_value=.76;l.new(t.outputs['Color'],hs.inputs['Color']);l.new(hs.outputs[0],n.get('Principled BSDF').inputs['Base Color'])
mesh('Resolute detailed oak relief front',[(-.94,D-.642,.10),(.94,D-.642,.10),(.94,D-.642,.82),(-.94,D-.642,.82)],[(0,1,2,3)],relief,[(709/2048,1-747/1024),(1327/2048,1-761/1024),(1340/2048,1-520/1024),(699/2048,1-510/1024)])
# Side panel frames on the desk's exposed flanks.
for side in [-1,1]:
 for yy in [D-.31,D+.29]:
  before=set(bpy.data.objects);frame('Pedestal side raised panel',0,0,.425,.39,.43,woodlight,.027);group_transform(before,(side*.929,yy,0),side*pi/2)
blotter=mat('Dark cordovan leather blotter',(.022,.043,.036),.46)
cube('Writing blotter',(0,D,.914),(.98,.64,.018),blotter,.006)
for i in range(5):cube('Desk stationery',(.12,D-.10,.929+i*.0007),(.23,.295,.0006),paper)
curve('Fountain pen',[(-.12,D-.21,.934),(-.08,D+.02,.934)],.006,black)
cube('Desk document tray',(.66,D+.09,.944),(.30,.33,.067),wooddark,.008)
cube('Tray documents',(.66,D+.09,.982),(.27,.30,.020),paper,.002)
# Telephone with display, buttons and handset.
o=cube('Office telephone base',(-.68,D+.17,.964),(.25,.22,.095),black,.026);o.rotation_euler.x=.1
screen=mat('Telephone LCD',(.10,.20,.19),.25)
o=cube('Telephone LCD panel',(-.68,D+.205,1.018),(.14,.069,.006),screen,.004);o.rotation_euler.x=.1
for i in range(3):
 for j in range(4):cube('Telephone keypad',(-.72+i*.036,D+.12-j*.022,1.018),(.022,.014,.005),ivory,.003)
curve('Telephone handset',[(-.797,D+.08,1.04),(-.82,D+.13,1.067),(-.82,D+.27,1.067),(-.795,D+.30,1.04)],.022,black)
curve('Coiled telephone cord',[(-.84+.012*sin(i*.7),D+.12-i*.003,1.015+.012*cos(i*.7)) for i in range(95)],.003,black)

def chair(x,y,angle,executive=False):
 before=set(bpy.data.objects)
 if executive:
  cyl('Chair brass pedestal',(0,0,.23),.036,.36,brass)
  for i in range(5):
   a=i*2*pi/5;curve('Five star chair base',[(0,0,.12),(.31*cos(a),.31*sin(a),.08)],.018,mahogany);sphere('Chair caster',(.31*cos(a),.31*sin(a),.056),(.027,.027,.027),black)
  cushion('Executive leather seat',(0,0,.52),(.60,.53,.13),leather,.075)
  o=cushion('Executive high leather back',(0,.24,.89),(.61,.14,.69),leather,.08);o.rotation_euler.x=-.08
  for s in [-1,1]:curve('Executive walnut arm',[(s*.32,-.25,.56),(s*.32,-.2,.72),(s*.32,.22,.72)],.024,mahogany)
 else:
  for xx in [-.23,.23]:
   for yy in [-.22,.22]:
    curve('Guest chair tapered leg',[(xx*1.13,yy*1.1,.035),(xx,yy,.46)],.022,mahogany)
  cube('Guest chair seat frame',(0,0,.43),(.54,.51,.08),mahogany,.016)
  cushion('Guest chair upholstered seat',(0,-.02,.48),(.47,.43,.08),fabric,.035)
  for xx in [-.25,.25]:cube('Guest chair back stile',(xx,.21,.77),(.039,.04,.61),mahogany,.008)
  for zz in [.58,.97]:cube('Guest chair back rail',(0,.21,zz),(.53,.051,.055),mahogany,.009)
  # Actual woven cane grid, open through the chair back.
  cane=mat('Natural woven cane '+str(x),(.35,.22,.095),.58)
  for i in range(29):
   xx=-.218+i*.0155;curve('Vertical cane weave',[(xx,.21+.003*sin(j*pi),.605+j*.011) for j in range(31)],.0018,cane)
  for j in range(24):
   zz=.61+j*.014;curve('Horizontal cane weave',[(-.22+i*.011,.207+.002*sin(i*pi),zz) for i in range(41)],.0018,cane)
  for s in [-1,1]:
   curve('Guest chair polished arm',[(s*.28,-.20,.48),(s*.28,-.20,.66),(s*.28,.22,.69)],.020,mahogany)
 group_transform(before,(x,y,0),angle)
chair(0,3.28,0,True)
chair(-1.47,2.0,-.10);chair(1.47,2.0,.10)

# Rear credenza, personal picture frames and matching bowed chests.
C=coll('04 | Antiques and decorative objects')
def photo_frame(x,y,z,w,h,idx):
 m=noise_mat('Small framed photograph '+str(idx),(.07,.10,.08),(.51,.41,.28),5,.7,0)
 plane_front('Framed desk photograph',x,y,z+h/2,w,h,m);frame('Small photograph frame',x,y-.005,z+h/2,w+.015,h+.015,brass,.013)
 cube('Picture stand',(x,y+.03,z),(.06,.12,.009),black,.003)
cube('Rear mahogany credenza',(0,4.45,.48),(1.87,.43,.72),mahogany,.012)
cube('Credenza polished top',(0,4.45,.857),(1.96,.48,.045),wood,.012)
for i in range(8):photo_frame(-.80+i*.23,4.20,.885,.16,.15+(i%3)*.02,i)
def chest(x,y,angle):
 before=set(bpy.data.objects)
 for xx in [-.49,.49]:
  for yy in [-.23,.23]:curve('Chest bracket foot',[(xx,yy,.25),(xx*1.03,yy*1.06,.06)],.036,mahogany)
 cube('Mahogany chest carcass',(0,0,.64),(1.13,.53,.87),mahogany,.018)
 cube('Chest overhanging top',(0,0,1.092),(1.20,.58,.043),wood,.009)
 for i in range(4):
  z=.33+i*.21;cube('Dovetailed drawer front',(0,-.276,z),(1.078,.044,.191),mahogany,.008)
  for xx in [-.33,.33]:
   sphere('Drawer brass escutcheon',(xx,-.305,z),(.032,.009,.023),brass)
   curve('Drawer drop handle',[(xx+.020*cos(a),-.324,z-.014+.013*sin(a)) for a in [2*pi*j/30 for j in range(31)]],.004,brass)
 group_transform(before,(x,y,0),angle)
chest(-3.04,3.19,.55);chest(3.04,3.19,-.55)
# Small dark bronze bronco and rider, inspired by the Remington on the chest.
bronze=noise_mat('Patinated bronze sculpture',(.017,.023,.018),(.055,.073,.055),15,.36,.002)
before=set(bpy.data.objects)
cube('Bronze sculpture marble base',(0,0,1.14),(.48,.25,.045),black,.012)
sphere('Bronco body',(0,0,1.43),(.17,.066,.085),bronze)
sphere('Bronco haunch',(.115,0,1.42),(.071,.066,.076),bronze)
o=sphere('Arched horse neck',(-.135,0,1.52),(.050,.048,.115),bronze);o.rotation_euler.y=-.5
o=sphere('Horse head',(-.20,0,1.60),(.070,.039,.043),bronze);o.rotation_euler.y=.35
for yy in [-.028,.028]:
 curve('Bronco lifted front leg',[(-.10,yy,1.40),(-.20,yy,1.31),(-.26,yy,1.38)],.014,bronze)
 curve('Bronco hind leg',[(.11,yy,1.40),(.19,yy,1.30),(.08,yy,1.17)],.016,bronze)
 sphere('Horse ear',(-.175,yy,1.649),(.009,.009,.024),bronze)
curve('Bronco tail',[(.15,0,1.46),(.25,0,1.53),(.28,0,1.47)],.017,bronze)
sphere('Bronze rider torso',(-.01,0,1.60),(.04,.029,.072),bronze)
sphere('Bronze rider head',(-.026,0,1.70),(.025,.023,.033),bronze)
o=cyl('Rider hat brim',(-.026,0,1.724),.050,.009,bronze);o.rotation_euler.y=.2
cyl('Rider hat crown',(-.026,0,1.742),.026,.033,bronze)
for yy in [-.053,.053]:curve('Rider leg',[(0,yy,1.57),(-.046,yy*1.2,1.44),(.026,yy*1.4,1.36)],.014,bronze)
curve('Rider outstretched arm',[(-.015,0,1.65),(-.12,-.04,1.70),(-.24,-.04,1.69)],.012,bronze)
curve('Rider reins arm',[(0,.02,1.65),(-.035,.045,1.60),(-.16,.025,1.57)],.013,bronze)
group_transform(before,(3.04,3.19,0),-.55)
# Real painting reproductions with multilayer gilt frames.
def painting(x,y,z,w,h,ma,angle):
 before=set(bpy.data.objects);plane_front('Oil on canvas',0,-.017,z,w,h,ma)
 for pad,thick,depth in [(.02,.014,-.03),(.075,.045,0),(.14,.018,.01)]:frame('Gilt picture frame',0,depth,z,w+pad,h+pad,gold,thick)
 group_transform(before,(x,y,0),angle)
painting(-3.26,3.67,2.41,.68,1.27,avemat,.70)
painting(3.26,3.67,2.52,.60,.75,libmat,-.70)

# Flag poles, sculpted hanging fabric, finials and tassels.
C=coll('05 | Ceremonial flags')
for idx,(x,y) in enumerate([(-1.27,4.20),(1.27,4.20)]):
 cyl('Flagpole weighted brass base',(x,y,.054),.18,.07,brass);cyl('Flagpole',(x,y,1.73),.013,3.40,brass)
 sphere('Flagpole eagle finial body',(x,y,3.49),(.035,.028,.074),gold)
 for s in [-1,1]:
  o=sphere('Finial spread wing',(x+s*.065,y,3.51),(.090,.012,.022),gold);o.rotation_euler.y=s*-.45
 nx=45;nz=80;v=[];uv=[]
 for j in range(nz+1):
  q=j/nz
  for i in range(nx+1):
   u=i/nx;xx=x+.53*u*(.70+.30*sin(q*pi))+.08*sin(q*pi)*u;yy=y-.06+.17*sin(u*pi*5.7+q*1.5)*u;zz=3.38-2.35*q-.38*u
   v.append((xx,yy,zz));uv.append((u,1-q))
 f=[(j*(nx+1)+i,j*(nx+1)+i+1,(j+1)*(nx+1)+i+1,(j+1)*(nx+1)+i) for j in range(nz) for i in range(nx)]
 o=mesh('US flag' if idx==0 else 'Presidential standard',v,f,usmat if idx==0 else presmat,uv)
 for p in o.data.polygons:p.use_smooth=True
 so=o.modifiers.new('Flag hem thickness','SOLIDIFY');so.thickness=.002
 curve('Gold flag fringe',[v[j*(nx+1)+nx] for j in range(nz+1)]+[v[nz*(nx+1)+i] for i in range(nx,-1,-1)],.009,gold)
 curve('Ceremonial cord',[(x+.02,y-.06,3.39),(x+.10,y-.1,3.15),(x+.13,y-.1,3.35)],.006,gold)

# Side tables, porcelain lamps, books, doors and fireplace complete the room.
C=coll('06 | Remaining room furnishings')
def lamp(x,y):
 cyl('Lamp table top',(x,y,.66),.40,.055,mahogany)
 cyl('Lamp table stem',(x,y,.35),.05,.56,mahogany)
 for i in range(3):
  a=i*2*pi/3;curve('Table tripod foot',[(x,y,.16),(x+.3*cos(a),y+.3*sin(a),.07)],.027,mahogany)
 cyl('Lamp brass foot',(x,y,.725),.14,.045,brass)
 lathe('Glazed porcelain lamp',[(.06,0),(.13,.05),(.145,.17),(.11,.32),(.046,.41),(.044,.46)],(x,y,.745),porcelain)
 cyl('Lamp neck',(x,y,1.25),.02,.13,brass)
 shade=mat('Lampshade warm linen',(.85,.76,.56),.85)
 p=shade.node_tree.nodes.get('Principled BSDF');p.inputs['Subsurface Weight'].default_value=.08
 lathe('Tapered linen lampshade',[(.30,0),(.21,.41),(.20,.41),(.29,0)],(x,y,1.30),shade)
 for z,r in [(1.30,.30),(1.71,.21)]:curve('Shade binding',[(x+r*cos(i*2*pi/100),y+r*sin(i*2*pi/100),z) for i in range(100)],.007,piping,True)
 sphere('Lamp finial',(x,y,1.75),(.022,.022,.035),brass)
 light=bpy.data.lights.new('Warm lamp bulb','POINT');light.energy=14;light.color=(1,.74,.43);light.shadow_soft_size=.12;o=bpy.data.objects.new('Warm lamp bulb',light);C.objects.link(o);o.location=(x,y,1.46)
lamp(-3.25,-1.95);lamp(3.25,-1.95)
for s in [-1,1]:
 before=set(bpy.data.objects)
 cube('Recessed painted door',(0,0,1.45),(1.05,.08,2.9),ivory,.007)
 for zz,hh in [(.57,.71),(1.83,1.35)]:
  frame('Door field panel',0,-.052,zz,.74,hh,white,.032)
 for xx in [-.60,.60]:cube('Door fluted architrave',(xx,-.04,1.5),(.13,.14,3.06),white,.006)
 cube('Door lintel',(0,-.04,3.02),(1.33,.15,.14),white,.008)
 sphere('Door brass knob',(.37,-.11,1.2),(.025,.04,.025),brass)
 group_transform(before,(s*4.20,.05,0),s*pi/2)
marble=noise_mat('White marble fireplace',(.69,.69,.64),(.9,.88,.81),4,.28,.005,(3,1,8))
cube('Fireplace dark firebox',(0,-5.28,.63),(1.60,.16,1.1),black,.018)
for x in [-.98,.98]:cube('White marble mantel jamb',(x,-5.11,.69),(.31,.35,1.35),marble,.013)
cube('Marble mantel frieze',(0,-5.11,1.30),(2.21,.37,.24),marble,.011)
cube('White marble mantel shelf',(0,-5.08,1.46),(2.40,.46,.11),marble,.016)
cube('Marble hearth',(0,-4.91,.035),(2.42,.87,.055),marble,.009)
for i in range(8):curve('Fireplace iron grate',[(-.67+i*.19,-4.99,.16),(-.67+i*.19,-4.99,.42)],.01,black)

# Outdoor shrubs, lawn and branching trees provide true parallax through the glass.
C=coll('07 | South lawn and lighting')
grass=noise_mat('Lawn',(.035,.065,.012),(.16,.24,.043),12,.93,.05)
cube('South lawn',(0,14,-.12),(70,50,.12),grass)
leafm=[mat('Garden foliage '+str(i),(.018+i*.011,.050+i*.017,.008+i*.004),.78) for i in range(6)]
bark=noise_mat('Garden tree bark',(.035,.019,.01),(.12,.075,.039),13,.95,.02)
for i in range(22):
 x=random.uniform(-18,18);y=random.uniform(10,22);h=random.uniform(5,10)
 curve('Distant garden trunk',[(x,y,0),(x+.18,y,h*.62),(x+.5,y,h)],.14,bark)
 for j in range(8):
  xx=x+random.uniform(-2,2);yy=y+random.uniform(-1.5,1.5);zz=h+random.uniform(-1.8,1.5)
  curve('Garden branch',[(x,y,h*.55),(xx,yy,zz)],.04,bark)
  vv=[];ff=[]
  for k in range(220):
   dx=random.gauss(0,.68);dy=random.gauss(0,.62);dz=random.gauss(0,.55);lc=Vector((xx+dx,yy+dy,zz+dz));ax=Vector((random.uniform(-1,1),random.uniform(-1,1),random.uniform(-.6,.6))).normalized()*random.uniform(.06,.13);ay=ax.cross(Vector((0,0,1))).normalized()*random.uniform(.025,.055);v0=len(vv);vv.extend([tuple(lc-ax),tuple(lc+ay),tuple(lc+ax),tuple(lc-ay)]);ff.append((v0,v0+1,v0+2,v0+3))
  mesh('Individual garden leaves',vv,ff,random.choice(leafm))

world=bpy.data.worlds.new('Soft outdoor morning environment');world.use_nodes=True;scene.world=world
n=world.node_tree.nodes;l=world.node_tree.links;env=n.new('ShaderNodeTexEnvironment');env.image=bpy.data.images.load(str(ROOT/'textures'/'garden.hdr'));l.new(env.outputs['Color'],n.get('Background').inputs['Color']);n.get('Background').inputs['Strength'].default_value=2.2
def area(name,loc,target,power,size,color):
 d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;d.color=color;o=bpy.data.objects.new(name,d);C.objects.link(o);o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();o.visible_camera=False;o.visible_glossy=False;o.visible_transmission=False;return o
area('Broad daylight through south windows',(0,6.1,3.9),(0,0,1.1),2300,5.0,(.89,.94,1))
area('Photographic ambient bounce',(0,-2.8,4.3),(0,2.5,.7),450,4.0,(1,.98,.93))
sun=bpy.data.lights.new('Low sun through the trees','SUN');sun.energy=2.5;sun.angle=.055;sun.color=(1,.93,.81);o=bpy.data.objects.new('Low sun through the trees',sun);C.objects.link(o);o.rotation_euler=Vector((5,-12,-10)).to_track_quat('-Z','Y').to_euler()

C=coll('08 | Cameras')
def camera(name,loc,target,lens):
 d=bpy.data.cameras.new(name);o=bpy.data.objects.new(name,d);C.objects.link(o);o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler();d.lens=lens;d.clip_end=200;return o
hero=camera('01 | Oval Office architectural view',(-.24,-4.65,1.88),(0,2.5,1.59),27)
hero.data.dof.use_dof=True;hero.data.dof.focus_distance=7.3;hero.data.dof.aperture_fstop=5.6
detail=camera('02 | Resolute desk and south windows',(-2.20,-.58,1.52),(.0,3.12,1.49),34)
wide=camera('03 | Room overview',(-3.3,-3.3,2.1),(.4,1.6,1.5),22)
scene.camera=hero
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=96;scene.cycles.use_denoising=True
scene.cycles.max_bounces=9;scene.cycles.diffuse_bounces=4;scene.cycles.glossy_bounces=4;scene.cycles.transmission_bounces=8
scene.render.resolution_x=1600;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGB'
scene.view_settings.view_transform='AgX';scene.view_settings.look='AgX - Medium High Contrast';scene.view_settings.exposure=.15
scene.render.film_transparent=False
scene['Project']='Oval Office | Obama-era reconstruction, inspired by 2010–2016 references'
scene['Accuracy']='Approximate reconstruction. Custom furnishings and carvings; not a survey or photogrammetric replica.'
scene['Author']='Created in Blender for Ainsley'
# Make the initial workspace open in the composed camera, with friendly collection names.
for screen in bpy.data.screens:
 for a in screen.areas:
  if a.type=='VIEW_3D':a.spaces.active.region_3d.view_perspective='CAMERA';a.spaces.active.shading.type='MATERIAL'
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Oval_Office.blend'))
print('SCENE_READY',len(scene.objects),'objects',flush=True)
if '--preview' in sys.argv:
 scene.render.resolution_x=900;scene.render.resolution_y=620;scene.cycles.samples=24
 scene.render.filepath=str(ROOT/'renders'/'preview.png');bpy.ops.render.render(write_still=True)
elif '--render' in sys.argv:
 scene.render.filepath=str(ROOT/'renders'/'Oval_Office_Hero.png');bpy.ops.render.render(write_still=True)
