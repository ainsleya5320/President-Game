"""Build the offline political atlas from Natural Earth source data.
Run fetch_world_atlas.py first. Build dependencies: Pillow, numpy, pyshp,
shapely, pyproj, mapbox-earcut. Outputs are used directly by the native Slate map.
"""
from pathlib import Path
import sys, json, math, hashlib
P=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(P/'Saved/AtlasTools'))
import numpy as np, shapefile, mapbox_earcut as earcut
from shapely.geometry import shape, Polygon, MultiPolygon, Point
from shapely.ops import transform
from pyproj import Transformer
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter
Image.MAX_IMAGE_PIXELS=150_000_000
SRC=P/'Saved/AtlasSources';OUT=P/'Content/FourYears/Atlas';OUT.mkdir(parents=True,exist_ok=True)
FWD=Transformer.from_crs('EPSG:4326','+proj=robin +datum=WGS84 +units=m',always_xy=True)
INV=Transformer.from_crs('+proj=robin +datum=WGS84 +units=m','EPSG:4326',always_xy=True)
X=17005833.33052523;Y=8625154.6651;W=6144;H=3116
def project(lon,lat):
 x,y=FWD.transform(lon,lat);return ((x+X)/(2*X),(Y-y)/(2*Y))
def project_geom(g):
 return transform(lambda x,y,z=None:((np.asarray(FWD.transform(x,y)[0])+X)/(2*X),(Y-np.asarray(FWD.transform(x,y)[1]))/(2*Y)),g)
def read(kind):return shapefile.Reader(str(next((SRC/kind).rglob('*.shp'))),encoding='utf-8')
EU=set('AUT BEL BGR HRV CYP CZE DNK EST FIN FRA DEU GRC HUN IRL ITA LVA LTU LUX MLT NLD POL PRT ROU SVK SVN ESP SWE'.split())
GULF=set('SAU ARE QAT KWT OMN BHR'.split())
GROUP={'CAN':'canada','BRA':'brazil','GBR':'uk','RUS':'russia','ZAF':'africa','IND':'india','CHN':'china','IDN':'indonesia','JPN':'japan','AUS':'australia'}
for k in EU:GROUP[k]='europe'
for k in GULF:GROUP[k]='gulf'
palette=[(206,188,145),(163,187,163),(201,166,143),(164,183,195),(192,185,156),(174,170,192),(191,172,146),(164,192,181),(204,185,179),(183,194,145),(181,174,153),(168,189,190),(198,180,157)]
countries=[]; geometries=[]
for sr in read('countries').iterShapeRecords():
 a=sr.record.as_dict(); geo=shape(sr.shape.__geo_interface__)
 if not geo.is_valid:geo=geo.buffer(0)
 geo=geo.simplify(.016,preserve_topology=True)
 pg=project_geom(geo)
 if not pg.is_valid:pg=pg.buffer(0)
 parts=list(pg.geoms) if isinstance(pg,MultiPolygon) else [pg]
 polys=[]
 for poly in parts:
  if poly.area<1e-10 or not isinstance(poly,Polygon):continue
  rings=[np.asarray(poly.exterior.coords)[:-1]]+[np.asarray(r.coords)[:-1] for r in poly.interiors]
  rings=[r for r in rings if len(r)>=3]
  verts=np.vstack(rings).astype(np.float64)
  indices=earcut.triangulate_float64(verts,np.cumsum([len(r) for r in rings],dtype=np.uint32))
  a1=verts[indices[1::3]]-verts[indices[0::3]];a2=verts[indices[2::3]]-verts[indices[0::3]]
  area=np.abs(a1[:,0]*a2[:,1]-a1[:,1]*a2[:,0]).sum()*.5
  if abs(area-poly.area)>max(1e-9,poly.area*.001):raise ValueError('Triangulation area mismatch '+a['NAME'])
  polys.append({'rings':[[[round(float(x),7),round(float(y),7)] for x,y in r] for r in rings],
                'v':np.round(verts,7).tolist(),'i':indices.tolist(),'bounds':list(poly.bounds)})
 code=a['ADM0_A3'];main=max(parts,key=lambda p:p.area)
 label=project(a['LABEL_X'],a['LABEL_Y'])
 if not pg.covers(Point(label)):label=tuple(main.representative_point().coords)[0]
 c={'id':code,'name':a['NAME_EN'] or a['ADMIN'],'short':a['NAME'],'continent':a['CONTINENT'],'region':GROUP.get(code,''),'color':palette[(int(a['MAPCOLOR13'])-1)%13],
    'label':list(label),'area':float(pg.area),'rank':a['LABELRANK'],'focus':list(main.bounds),'polygons':polys}
 countries.append(c);geometries.append(pg)
print('Projected',len(countries),'countries / map units',flush=True)
# Actual point settlements only. No country-centroid markers are exported.
cities=[]
for sr in read('cities').iterShapeRecords():
 a=sr.record.as_dict()
 if not (a['adm0cap'] or a['worldcity'] or a['scalerank']<=3):continue
 # The city release uses ISO SSD for Wau; the boundary release uses ADM0 SDS.
 code={'SSD':'SDS'}.get(a['adm0_a3'],a['adm0_a3'])
 cities.append({'name':' '.join(a['name'].split()),'country':code,'capital':bool(a['adm0cap']),'rank':a['scalerank'],'p':list(project(a['longitude'],a['latitude'])),'lon':a['longitude'],'lat':a['latitude']})
# Diplomacy connects named cities; Brussels is the EU negotiating hub, not a country dot.
hubs={'usa':'Washington, D.C.','canada':'Ottawa','brazil':'Brasilia','uk':'London','europe':'Brussels','russia':'Moscow','gulf':'Riyadh','africa':'Pretoria','india':'New Delhi','china':'Beijing','indonesia':'Jakarta','japan':'Tokyo','australia':'Canberra'}
for c in cities:
 for group,name in hubs.items():
  if c['name'].replace('Brasília','Brasilia')==name or (group=='usa' and c['name'].startswith('Washington')):c['hub']=group
for c in countries:
 caps=[x for x in cities if x['country']==c['id'] and x['capital']]
 c['capital']=', '.join(x['name'] for x in caps)
# Project relief via inverse mapping in bounded strips.
srcfile=next((SRC/'relief').rglob('*.tif'));raw=np.asarray(Image.open(srcfile).convert('RGB'))
rh,rw=raw.shape[:2];relief=np.zeros((H,W,3),np.uint8)
for top in range(0,H,64):
 bottom=min(H,top+64);gx,gy=np.meshgrid(np.linspace(-X,X,W),(Y-(np.arange(top,bottom)+.5)*2*Y/H))
 lon,lat=INV.transform(gx,gy);valid=np.isfinite(lon)&np.isfinite(lat)&(abs(lon)<=180)&(abs(lat)<=90)
 sx=np.clip(np.nan_to_num((lon+180)/360*(rw-1),nan=0,posinf=0,neginf=0).astype(int),0,rw-1)
 sy=np.clip(np.nan_to_num((90-lat)/180*(rh-1),nan=0,posinf=0,neginf=0).astype(int),0,rh-1)
 relief[top:bottom]=raw[sy,sx]
 relief[top:bottom][~valid]=(19,43,60)
del raw
land=Image.new('L',(W,H));ld=ImageDraw.Draw(land)
political=Image.new('RGB',(W,H),(26,57,75));pd=ImageDraw.Draw(political)
def pixels(r):return [(round(x*(W-1)),round(y*(H-1))) for x,y in r]
for c in countries:
 for poly in c['polygons']:
  rings=poly['rings'];ld.polygon(pixels(rings[0]),fill=255);pd.polygon(pixels(rings[0]),fill=tuple(c['color']))
  for hole in rings[1:]:ld.polygon(pixels(hole),fill=0);pd.polygon(pixels(hole),fill=(26,57,75))
# Detailed lake silhouettes and river network.
water=[]
for sr in read('lakes').iterShapeRecords():
 g=project_geom(shape(sr.shape.__geo_interface__).simplify(.02))
 for poly in (list(g.geoms) if isinstance(g,MultiPolygon) else [g]):
  if isinstance(poly,Polygon):
   r=list(poly.exterior.coords);water.append(r);ld.polygon(pixels(r),fill=0);pd.polygon(pixels(r),fill=(26,57,75))
mask=np.asarray(land)>0
rr=relief.astype(np.float32)
# Ocean uses the same projected ocean mask, with a fine continental-shelf halo.
shelf=np.asarray(land.filter(ImageFilter.GaussianBlur(14))).astype(np.float32)/255
ocean=np.zeros_like(rr);ocean[:]=(25,55,74);ocean+=shelf[:,:,None]*np.array((13,19,17))
terrain=np.where(mask[:,:,None],rr*.88+np.array((12,16,7)),ocean)
lum=rr.mean(axis=2);shade=np.clip(.62+lum/420,.62,1.12)
pa=np.asarray(political).astype(np.float32);pa=pa*shade[:,:,None]
pa=np.where(mask[:,:,None],pa,ocean)
terrain=Image.fromarray(np.uint8(np.clip(terrain,0,255)))
political=Image.fromarray(np.uint8(np.clip(pa,0,255)))
for img in (terrain,political):
 draw=ImageDraw.Draw(img)
 for c in countries:
  for poly in c['polygons']:
   for ring in poly['rings']:
    pts=pixels(ring);draw.line(pts+[pts[0]],fill=(43,62,59),width=2)
 for sr in read('rivers').iterShapeRecords():
  a=sr.record.as_dict()
  if a.get('scalerank',9)>4:continue
  for line in (shape(sr.shape.__geo_interface__).geoms if sr.shape.shapeType==23 else [shape(sr.shape.__geo_interface__)]):
   if line.geom_type=='MultiLineString':lines=list(line.geoms)
   else:lines=[line]
   for l in lines:
    if l.geom_type!='LineString':continue
    pts=[project(x,y) for x,y in l.coords];draw.line(pixels(pts),fill=(71,126,145),width=1)
 for lake in water:draw.polygon(pixels(lake),fill=(26,57,75))
# Low-contrast graticules are separate runtime vector features and can be switched off.
grid=[]
for lon in range(-180,181,30):grid.append([list(project(lon,float(lat))) for lat in np.linspace(-89.9,89.9,121)])
for lat in range(-60,61,30):grid.append([list(project(float(lon),lat)) for lon in np.linspace(-180,180,241)])
data={'version':1,'projection':'Robinson','aspect':X/Y,'countries':countries,'cities':cities,'grid':grid,
      'sources':json.loads((SRC/'sources.json').read_text()),'borders':'Natural Earth de facto boundary convention, version 5.1.1'}
(OUT/'world.json').write_text(json.dumps(data,separators=(',',':'),ensure_ascii=False),encoding='utf-8')
for name,img in [('political',political),('terrain',terrain)]:
 img.save(OUT/(name+'.png'),optimize=True)
 img.resize((1536,779),Image.Resampling.LANCZOS).save(P/'Saved'/('atlas-'+name+'-preview.png'))
stats={'countries':len(countries),'cities':len(cities),'vertices':sum(len(p['v']) for c in countries for p in c['polygons']),'triangles':sum(len(p['i'])//3 for c in countries for p in c['polygons']),'projection':'Robinson','texture':[W,H]}
(OUT/'build.json').write_text(json.dumps(stats,indent=2))
print(json.dumps(stats,indent=2))
