"""Idempotent cabin detailing pass; run after ensure_walkable.py on the master blend."""
import bpy, math, shutil
from pathlib import Path
from mathutils import Vector
P=Path(__file__).parent
S=bpy.context.scene
assert S.get('Walkability revision'), 'Apply the walkable layout first'
for c in list(bpy.data.collections):
    if c.name.startswith(('09 |','10 |')):
        for o in list(c.objects): bpy.data.objects.remove(o,do_unlink=True)
        bpy.data.collections.remove(c)
for o in list(S.objects):
    if o.name.startswith(('Hall outboard wall','CEILING | Hall soffit','Hall overhead diffuser')):
        bpy.data.objects.remove(o,do_unlink=True)

def collection(name):
    c=bpy.data.collections.new(name);S.collection.children.link(c);return c
C=collection('09 | Sculpted aircraft passage')
ivory=bpy.data.materials['Warm ivory cabin leather']
cream=bpy.data.materials['Window surround enamel']
gold=bpy.data.materials['Champagne satin metal']
wood=bpy.data.materials['Bookmatched walnut veneer']
dark=bpy.data.materials['Dark bronze reveals']
paper=bpy.data.materials['Warm stationery']
navy=bpy.data.materials['Navy textile']

def mesh(name,verts,faces,ma,uv=None,smooth=False):
    me=bpy.data.meshes.new(name);me.from_pydata(verts,[],faces);me.update()
    o=bpy.data.objects.new(name,me);C.objects.link(o);me.materials.append(ma)
    layer=me.uv_layers.new()
    for poly in me.polygons:
        poly.use_smooth=smooth
        for li in poly.loop_indices:
            vi=me.loops[li].vertex_index
            layer.data[li].uv=uv[vi] if uv else (verts[vi][1]*.7,verts[vi][2]*.7)
    return o

def cube(name,loc,size,ma,bevel=.004):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name
    for c in list(o.users_collection):c.objects.unlink(o)
    C.objects.link(o);o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(ma)
    if bevel:
        m=o.modifiers.new('Soft edge highlights','BEVEL');m.width=bevel;m.segments=3
        o.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL')
    return o

def curve(name,pts,r,ma,closed=False):
    cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.bevel_depth=r;cu.bevel_resolution=3
    sp=cu.splines.new('POLY');sp.points.add(len(pts)-1);sp.use_cyclic_u=closed
    for p,co in zip(sp.points,pts):p.co=(*co,1)
    o=bpy.data.objects.new(name,cu);C.objects.link(o);cu.materials.append(ma);return o

def image_material(name,path,emission=0):
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True
    m.node_tree.nodes.clear();n=m.node_tree.nodes;l=m.node_tree.links
    out=n.new('ShaderNodeOutputMaterial');bs=n.new('ShaderNodeBsdfPrincipled');l.new(bs.outputs[0],out.inputs[0])
    bs.inputs['Roughness'].default_value=.65
    tex=n.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(path),check_existing=True)
    l.new(tex.outputs['Color'],bs.inputs['Base Color'])
    if emission:l.new(tex.outputs['Color'],bs.inputs['Emission Color']);bs.inputs['Emission Strength'].default_value=emission
    return m

def rounded(w,h,r):
    result=[]
    for cy,cz,a in [(w/2-r,h/2-r,0),(-w/2+r,h/2-r,math.pi/2),(-w/2+r,-h/2+r,math.pi),(w/2-r,-h/2+r,3*math.pi/2)]:
        for i in range(16):
            t=a+i*math.pi/30;result.append((cy+r*math.cos(t),cz+r*math.sin(t)))
    return result

# The wall rolls inward above shoulder height. Separate bands retain conservative
# floor-plan collision tests without projecting the overhead ceiling across the aisle.
profile=[(3.59,0),(3.58,.82),(3.56,1.10),(3.53,1.50),(3.47,1.80),(3.38,1.98),
         (3.29,2.10),(3.15,2.24),(2.97,2.36),(2.77,2.45),(2.55,2.50),(2.31,2.53),(2.08,2.53)]
def wallx(z):
    for (x0,z0),(x1,z1) in zip(profile,profile[1:]):
        if z<=z1:return x0+(x1-x0)*(z-z0)/(z1-z0)
    return profile[-1][0]

stations=[1.18,3.29,5.40,7.51,9.62,11.73]
cloud=image_material('Aircraft window cloud panorama',P/'textures'/'cloud-deck.jpg',.65)
for k,y in enumerate(stations):
    ya=-.06 if k==0 else (y+stations[k-1])/2
    yb=12.98 if k==5 else (y+stations[k+1])/2
    # Lower wall bands and curved upper shoulder / headliner.
    for a,b in [(0,.82),(.82,1.02)]:
        # Highest band needs both x positions where the crown becomes level.
        xa,xb=wallx(a),wallx(b)
        v=[(xa,ya,a),(xa,yb,a),(xb,yb,b),(xb,ya,b)]
        o=mesh('CEILING | Fuselage shoulder' if a>=1.98 else 'Hall lower curved panel',v,[(0,1,2,3)],ivory)
        so=o.modifiers.new('Panel thickness','SOLIDIFY');so.thickness=.025
    # Shared vertices and smooth normals produce a continuous curved headliner.
    crown=profile[5:]+[(2.04,2.53)]
    verts=[(x,yy,z) for yy in [ya,yb] for x,z in crown];nc=len(crown)
    mesh('CEILING | Continuous fuselage headliner',verts,[(i,i+1,nc+i+1,nc+i) for i in range(nc-1)],ivory,smooth=True)
    # A rounded opening, not a window painted over a solid wall.
    outer=rounded(yb-ya,.96,0);inner=rounded(.46,.72,.21)
    n=len(inner);verts=[]
    for yy,z in outer:verts.append((wallx(z+1.5),(ya+yb)/2+yy,z+1.5))
    for yy,z in inner:verts.append((wallx(z+1.5),y+yy,z+1.5))
    mesh('Hall window belt with real aperture',verts,[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)],ivory)
    rings=[]
    for w,h,r,offset in [(.55,.81,.245,-.006),(.46,.72,.21,-.023),(.38,.62,.18,.065)]:
        rings.extend([(wallx(z+1.5)+offset,y+yy,z+1.5) for yy,z in rounded(w,h,r)])
    mesh('Sculpted oval window reveal',rings,[(j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i) for j in range(2) for i in range(n)],cream,smooth=True)
    curve('Oval window inner seal',[(wallx(z+1.5)+.062,y+yy,z+1.5) for yy,z in rounded(.385,.625,.182)],.008,dark,True)
    cube('Window shade pull',(wallx(1.83)+.03,y,1.83),(.028,.105,.022),gold)
    # Expansion ribs communicate the repeated aircraft frames without heavy bars.
    curve('CEILING | Curved fuselage seam',[(x-.008,yb,z) for x,z in profile if z>=1.98],.004,cream)
    curve('Hall vertical panel joint',[(wallx(z)-.002,yb,z) for z in [1.02,1.5,1.8,1.98]],.0018,dark)
    d=bpy.data.lights.new('Oval window daylight','AREA');d.energy=38;d.color=(.80,.9,1);d.shape='RECTANGLE';d.size=.40;d.size_y=.65
    o=bpy.data.objects.new('Oval window daylight',d);C.objects.link(o);o.location=(3.42,y,1.55);o.rotation_euler=(Vector((2.2,y,1.2))-o.location).to_track_quat('-Z','Y').to_euler()

# A continuous sky backing also fills the windows at grazing viewing angles.
# Independent small cards leave black gaps when looking down the corridor.
mesh('Cloud view through passage windows',[(3.75,-20,-1),(3.75,35,-1),(3.75,35,5),(3.75,-20,5)],[(0,1,2,3)],cloud,[(0,0),(7,0),(7,1),(0,1)])
lightmat=bpy.data.materials.get('Luminous champagne cove') or bpy.data.materials.new('Luminous champagne cove')
lightmat.use_nodes=True;bs=lightmat.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(1,.74,.4,1);bs.inputs['Emission Color'].default_value=(1,.78,.48,1);bs.inputs['Emission Strength'].default_value=3
for x,z in [(2.24,2.505),(3.20,2.185)]:
    cube('CEILING | Champagne cove housing',(x,6.46,z),(.032,12.86,.020),gold)
    cube('CEILING | Continuous warm cove',(x-.008,6.46,z-.012),(.014,12.84,.007),lightmat,.002)

C=collection('10 | Presidential seals and gallery')
assets=P.parent/'Unreal Projects'/'OvalOffice'/'Prototype'/'assets'
art={}
for key,file in [('Washington','washington-portrait.png'),('American landscape','river-landscape.png'),('Liberty','statue-liberty.png')]:
    dest=P/'textures'/file
    if not dest.exists():shutil.copy2(assets/file,dest)
    art[key]=image_material('Gallery | '+key,dest)
sealmat=image_material('Presidential seal gallery plaque',P/'textures'/'presidential-seal.png')

def basis(center,normal):
    n=Vector(normal);v=Vector((0,0,1));h=v.cross(n)
    return Vector(center),h,v,n

def frame(title,center,normal,w,h,ma):
    c,u,v,n=basis(center,normal)
    def box(label,cu,cv,ww,hh,depth,offset,material):
        loc=c+u*cu+v*cv+n*offset
        obj=cube(title+' | '+label,loc,(ww,depth,hh),material)
        obj.rotation_euler[2]=math.atan2(u.y,u.x)
    box('walnut backing',0,0,w+.09,h+.09,.028,.014,wood)
    box('linen mount',0,0,w+.04,h+.04,.004,.031,paper)
    for side in [-1,1]:
        box('champagne vertical',side*(w/2+.036),0,.022,h+.09,.013,.035,gold)
        box('champagne horizontal',0,side*(h/2+.036),w+.09,.022,.013,.035,gold)
    verts=[tuple(c+u*a*w/2+v*b*h/2+n*.038) for a,b in [(-1,-1),(1,-1),(1,1),(-1,1)]]
    mesh(title+' | artwork',verts,[(0,1,2,3)],ma,[(0,0),(1,0),(1,1),(0,1)])

def seal(title,center,normal,r):
    c,u,v,n=basis(center,normal)
    # Gold outer bead and a navy substrate frame the full-color emblem.
    for radius,offset,ma in [(r+.026,.008,gold),(r+.014,.018,navy),(r,.025,sealmat)]:
        verts=[tuple(c+n*offset)];uv=[(.5,.5)]
        for i in range(96):
            t=i*2*math.pi/96;verts.append(tuple(c+u*math.cos(t)*radius+v*math.sin(t)*radius+n*offset));uv.append((.5+.5*math.cos(t),.5+.5*math.sin(t)))
        mesh(title,verts,[(0,i+1,(i+1)%96+1) for i in range(96)],ma,uv)
    curve(title+' | raised brass rim',[tuple(c+u*math.cos(i*math.tau/96)*(r+.02)+v*math.sin(i*math.tau/96)*(r+.02)+n*.025) for i in range(96)],.005,gold,True)

for i,y in enumerate([.95,2.0,4.96,8.25,10.05,11.75]):
    key=['American landscape','Liberty','Washington','American landscape','Liberty','Washington'][i]
    frame('Passage gallery '+str(i+1),(2.137,y,1.56),(1,0,0),.48 if key=='Washington' else .68,.63 if key=='Washington' else .46,art[key])
for y,normal in [(.078,(0,1,0)),(12.84,(0,-1,0))]:seal('Passage presidential seal',(2.77,y,1.62),normal,.23)
frame('Office presidential portrait',(-.45,.075,1.68),(0,1,0),.55,.73,art['Washington'])
frame('Office landscape',(-1.68,.075,1.68),(0,1,0),.73,.49,art['American landscape'])
frame('Office Liberty',(.78,.075,1.68),(0,1,0),.56,.69,art['Liberty'])
seal('Office presidential seal',(-2.76,.075,1.72),(0,1,0),.22)
for y,key in [(1.03,'American landscape'),(2.19,'Liberty')]:
    frame('Office lounge '+key,(1.962,y,1.77),(-1,0,0),.72,.47,art[key])
seal('Conference presidential seal',(-.57,12.815,2.17),(0,-1,0),.18)
for x,key in [(-2.05,'American landscape'),(.94,'Liberty')]:
    frame('Conference '+key,(x,12.815,1.54),(0,-1,0),.72,.51,art[key])
for y,key in [(8.45,'Washington'),(10.65,'American landscape')]:
    frame('Conference gallery '+key,(1.962,y,1.73),(-1,0,0),.62 if key=='Washington' else .93,.76 if key=='Washington' else .61,art[key])

cam=bpy.data.objects['03 | Connecting hallway'];cam.location=(2.70,.63,1.63);cam.rotation_euler=(Vector((2.90,8.5,1.64))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.lens=22
S['Cabin detailing revision']=1
S['Game integration']='Native Unreal first-person level with collision and fixed WASD controls; geometry adapted for a 68 cm capsule.'
bpy.ops.file.pack_all()
bpy.ops.wm.save_as_mainfile(filepath=str(P/'Air_Force_One.blend'))
print('CABIN_REFINEMENT_COMPLETE',flush=True)
