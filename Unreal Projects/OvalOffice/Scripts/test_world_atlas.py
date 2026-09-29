"""Validate shipped atlas geometry, actual city anchors, and political grouping."""
from pathlib import Path
import json, math, sys
P=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(P/'Saved/AtlasTools'))
from pyproj import Transformer
from shapely.geometry import Polygon, Point
from PIL import Image
d=json.loads((P/'Content/FourYears/Atlas/world.json').read_text(encoding='utf-8'))
countries={c['id']:c for c in d['countries']}
geom={k:[Polygon(x['rings'][0],x['rings'][1:]) for x in c['polygons']] for k,c in countries.items()}
proj=Transformer.from_crs('EPSG:4326','+proj=robin +datum=WGS84 +units=m',always_xy=True)
def project(lon,lat):
 x,y=proj.transform(lon,lat);return ((x+17005833.33052523)/34011666.66105046,(8625154.6651-y)/17250309.3302)
def hit(lon,lat):
 p=Point(project(lon,lat))
 return [k for k,g in geom.items() if any(poly.contains(p) for poly in g)]
checks=[]
def check(name,ok):
 assert ok,name
 checks.append(name)
check('258 distinct geographic units',len(countries)==258)
check('Robinson projection metadata',d['projection']=='Robinson')
cases=[
 ('Washington',-77.0369,38.9072,'USA'),('Paris',2.3522,48.8566,'FRA'),
 ('Berlin',13.405,52.52,'DEU'),('Brussels',4.3517,50.8503,'BEL'),
 ('London',-.1276,51.5072,'GBR'),('New Delhi',77.209,28.6139,'IND'),
 ('Beijing',116.4074,39.9042,'CHN'),('Tokyo',139.6917,35.6895,'JPN'),
 ('Riyadh',46.6753,24.7136,'SAU'),('Doha',51.531,25.286,'QAT'),
 ('Jakarta',106.8456,-6.2088,'IDN'),('Canberra',149.13,-35.28,'AUS'),
 ('Maseru enclave',27.4833,-29.3167,'LSO'),('Kaliningrad exclave',20.51,54.71,'RUS'),
 ('Anchorage detached Alaska',-149.9,61.22,'USA'),('Honolulu island',-157.8583,21.3069,'USA'),
 ('Suva antimeridian islands',178.441707,-18.133016,'FJI'),('Wellington',174.7762,-41.2865,'NZL'),
 ('Atlantic ocean',-35,25,None),('Pacific ocean',-145,-15,None)]
for name,lon,lat,expected in cases:
 found=hit(lon,lat)
 check(name+' territory hit',found==[expected] if expected else not found)
expected={'usa':'USA','canada':'CAN','brazil':'BRA','uk':'GBR','europe':'BEL','russia':'RUS','gulf':'SAU','africa':'ZAF','india':'IND','china':'CHN','indonesia':'IDN','japan':'JPN','australia':'AUS'}
hubs=[c for c in d['cities'] if c.get('hub')]
check('Exactly one actual city per trade hub',len(hubs)==len(expected) and len({c['hub'] for c in hubs})==len(expected))
for c in hubs:
 check(c['hub']+' city and country',c['country']==expected[c['hub']] and bool(c['name']) and c['capital'])
 check(c['hub']+' projected anchor',math.dist(project(c['lon'],c['lat']),c['p'])<1e-7)
check('EU national borders stay independent',all(countries[k]['region']=='europe' for k in ['FRA','DEU','BEL','POL','ESP','ITA']) and countries['GBR']['region']=='uk')
check('Gulf national borders stay independent',all(countries[k]['region']=='gulf' for k in ['SAU','ARE','QAT','KWT','OMN','BHR']))
check('Unimplemented countries stay informational',countries['MEX']['region']=='' and countries['USA']['region']=='')
check('Searchable city names and finite world coordinates',all(c['name'] and c['country'] in countries and all(math.isfinite(v) and 0<=v<=1 for v in c['p']) for c in d['cities']))
polys=[p for c in countries.values() for p in c['polygons']]
check('All polygon indices fit 16-bit Slate meshes',all(len(p['v'])<65536 and len(p['i'])%3==0 and all(0<=i<len(p['v']) for i in p['i']) for p in polys))
check('Political and terrain images align',all(Image.open(P/'Content/FourYears/Atlas'/f).size==(6144,3116) for f in ['political.png','terrain.png']))
check('Detailed geometry rather than sketch outlines',sum(len(p['v']) for p in polys)>100000)
result={'passed':len(checks),'checks':checks,'countries':len(countries),'cities':len(d['cities']),'max_mesh_vertices':max(len(p['v']) for p in polys)}
(P/'Saved/AtlasValidation.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='checks'},indent=2))
