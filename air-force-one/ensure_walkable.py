"""Widen architecture, preserve furniture scale, then test a human-size controller."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector,Matrix
import numpy as np
from collections import deque
P=Path(__file__).parent;s=bpy.context.scene
def remap(v,anchors):
 for (a,b),(c,d) in zip(anchors,anchors[1:]):
  if v<=c:return b+(v-a)*(d-b)/(c-a)
 a,b=anchors[-2];c,d=anchors[-1];return d+(v-c)*(d-b)/(c-a)
def fx(x):return remap(x,[(-2.56,-3.46),(1.39,2.05),(2.61,3.55)])
def fy(y):return remap(y,[(0,0),(3.05,2.94),(4.10,4.26),(4.66,4.85),(5.18,5.45),(5.55,5.79),(6.60,7.11),(11.66,12.90)])
if not s.get('Walkability revision'):
 bpy.context.view_layer.update()
 for o in list(s.objects):
  col=' '.join(c.name for c in o.users_collection)
  if col.startswith(('01 |','02 |','03 |')) and o.type in {'MESH','CURVE'}:
   # Apply in world space so corner joins and real door openings stay aligned.
   world=o.matrix_world.copy()
   if o.type=='MESH':
    o.data=o.data.copy()
    for v in o.data.vertices:
     p=world@v.co;v.co=(fx(p.x),fy(p.y),p.z)
   else:
    o.data=o.data.copy()
    for spline in o.data.splines:
     for point in spline.points:
      p=world@Vector(point.co[:3]);point.co=(fx(p.x),fy(p.y),p.z,1)
   o.matrix_world=Matrix.Identity(4)
  elif col.startswith(('01 |','02 |','03 |')) and o.type in {'FONT','LIGHT'}:
   o.location.x=fx(o.location.x);o.location.y=fy(o.location.y)
  elif col.startswith('04 |'):
   center=sum((o.matrix_world@Vector(v) for v in o.bound_box),Vector())/8
   if o.name.startswith(('Window-side','Credenza','Flush cabinet','Cabinet latch')):o.location.x-=.9
   elif o.name.startswith(('Sofa','Tufted sofa')) or (o.name.startswith('Leather covered tuft') and center.x>.35):o.location.x+=.66
  elif col.startswith('05 |'):
   o.location.y+=.4
   if o.name.startswith(('Conference credenza','Conference cabinet')):o.location.x-=.9
   if o.name.startswith(('Monitor bulkhead','Briefing display')):o.location.y+=.84
  elif col.startswith('06 |'):
   center=sum((o.matrix_world@Vector(v) for v in o.bound_box),Vector())/8
   if center.y>5:o.location.y+=.4
   if center.x<-2:o.location.x-=.9
  elif col.startswith('07 |'):
   if o.type=='LIGHT':
    o.location.x=fx(o.location.x);o.location.y=fy(o.location.y);o.data.energy*=1.15
   elif o.type=='MESH' and not o.name.startswith('EXTERIOR'):
    o.location.x=fx(o.location.x);o.location.y=fy(o.location.y)
 for o in s.objects:
  if o.type=='EMPTY':o.location.x=fx(o.location.x);o.location.y=fy(o.location.y)
 poses={'01 | Presidential office':((1.42,4.46,1.63),(-.85,1.76,1.06)),
 '02 | Conference room':((1.56,5.82,1.65),(-.60,9.05,1.04)),
 '03 | Connecting hallway':((2.77,1,1.65),(2.74,8.5,1.42)),
 '04 | Three-room cutaway':((14,19,16),(-.1,6.4,0))}
 for name,(loc,target) in poses.items():
  o=bpy.data.objects[name];o.location=loc;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
 s['Walkability revision']=1
 s['Walkability auxiliary positions']=1
 s['Game integration']='Widened and clearance-tested for a 0.68 m wide, 1.80 m tall player. Engine collision/navigation still requires integration.'
 bpy.context.view_layer.update()

if not s.get('Walkability auxiliary positions'):
 for o in s.objects:
  col=' '.join(c.name for c in o.users_collection)
  if col.startswith(('01 |','02 |','03 |')) and o.type in {'FONT','LIGHT'}:
   o.location.x=fx(o.location.x);o.location.y=fy(o.location.y)
 s['Walkability auxiliary positions']=1
 bpy.context.view_layer.update()

# Move every component of each cabinet lamp together, including fine seam curves.
# A footprint-edge test alone can split the components on opposite sides of x=-2.
if not s.get('Walkability lamp assemblies'):
 for o in s.objects:
  if o.name.startswith(('Lamp ','Pleated linen lampshade','Linen shade seam')):
   if -2.5<o.location.x<-1.8:o.location.x-=.9
   if 5.3<o.location.y<5.9:o.location.y+=.4
 s['Walkability lamp assemblies']=1
 bpy.context.view_layer.update()

# Conservative footprint test: every visible solid intersecting the standing body
# is projected to its convex footprint. This includes chair arms, lamps and handrails.
def hull(points):
 points=sorted(set((round(x,5),round(y,5)) for x,y in points))
 def cross(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
 if len(points)<3:return points
 lo=[];hi=[]
 for p in points:
  while len(lo)>1 and cross(lo[-2],lo[-1],p)<=0:lo.pop()
  lo.append(p)
 for p in reversed(points):
  while len(hi)>1 and cross(hi[-2],hi[-1],p)<=0:hi.pop()
  hi.append(p)
 return lo[:-1]+hi[:-1]
deps=bpy.context.evaluated_depsgraph_get();solids=[]
for o in s.objects:
 if o.type not in {'MESH','CURVE','FONT'} or o.hide_render or o.name.startswith('EXTERIOR'):continue
 ev=o.evaluated_get(deps);me=ev.to_mesh()
 if not me:continue
 verts=np.array([tuple(ev.matrix_world@v.co) for v in me.vertices]);ev.to_mesh_clear()
 if len(verts)==0 or verts[:,2].max()<.12 or verts[:,2].min()>1.80:continue
 poly=hull(verts[:,:2]);
 if len(poly)>=3:solids.append({'name':o.name,'polygon':poly})
step=.05;radius=.34;margin=.02;clear=radius+margin
xs=np.arange(-3.40,3.50,step);ys=np.arange(.10,12.83,step);xx,yy=np.meshgrid(xs,ys);blocked=np.zeros(xx.shape,dtype=bool)
for item in solids:
 poly=np.array(item['polygon']);xmin,ymin=poly.min(axis=0)-clear;xmax,ymax=poly.max(axis=0)+clear
 ix=np.where((xs>=xmin)&(xs<=xmax))[0];iy=np.where((ys>=ymin)&(ys<=ymax))[0]
 if not len(ix) or not len(iy):continue
 x=xx[np.ix_(iy,ix)];y=yy[np.ix_(iy,ix)];inside=np.ones(x.shape,dtype=bool);dist=np.full(x.shape,np.inf)
 for a,b in zip(poly,np.roll(poly,-1,axis=0)):
  dx,dy=b-a;inside&=((x-a[0])*dy-(y-a[1])*dx)<=1e-8
  t=np.clip(((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy),0,1)
  dist=np.minimum(dist,(x-a[0]-t*dx)**2+(y-a[1]-t*dy)**2)
 blocked[np.ix_(iy,ix)]|=inside|(dist<=clear**2)
def cell(p):return (int(round((p[1]-ys[0])/step)),int(round((p[0]-xs[0])/step)))
start=(2.77,3.6);root=cell(start);assert not blocked[root],'Hall player start is blocked'
visited={root:None};q=deque([root])
while q:
 a=q.popleft()
 for di,dj in [(1,0),(-1,0),(0,1),(0,-1)]:
  b=(a[0]+di,a[1]+dj)
  if 0<=b[0]<len(ys) and 0<=b[1]<len(xs) and b not in visited and not blocked[b]:visited[b]=a;q.append(b)
targets={'office entrance':(1.6,3.6),'desk and sofa aisle':(.76,2.0),'office lounge':(.76,1.02),'conference entrance':(1.40,6.4),'conference right aisle':(1.50,9.0),'conference left aisle':(-2.44,9.0),'conference far end':(-.58,12.12)}
paths={};failed=[]
for name,p in targets.items():
 end=cell(p)
 if end not in visited:failed.append(name);continue
 path=[]
 while end is not None:path.append([round(float(xs[end[1]]),3),round(float(ys[end[0]]),3)]);end=visited[end]
 paths[name]=list(reversed(path))
spawns={o.name:cell((o.location.x,o.location.y)) in visited for o in s.objects if o.name.startswith('PLAYER_START_')}
report={'player_width_m':radius*2,'player_height_m':1.8,'additional_radial_margin_m':margin,'grid_step_m':step,'tested_against':'Evaluated Blender geometry, conservatively projected into the standing body height; convex footprint per object','hall_clear_width_m':round(fx(2.54)-fx(1.48)-.036,3),'door_clear_width_m':round(fy(4.10-.0225)-fy(3.05+.0225),3),'reachable_destinations':list(paths),'unreachable_destinations':failed,'all_routes_pass':not failed and all(spawns.values()),'player_starts_clear_and_connected':spawns,'paths':paths,'revision':1}
(P/'walkability_report.json').write_text(json.dumps(report,indent=2))
(P/'collision_footprints.json').write_text(json.dumps(solids))
print('WALKABILITY',json.dumps({k:v for k,v in report.items() if k!='paths'}),flush=True)
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(P/'Air_Force_One.blend'))
if not report['all_routes_pass']:raise RuntimeError('A walking destination or player start is blocked: '+', '.join(failed))
