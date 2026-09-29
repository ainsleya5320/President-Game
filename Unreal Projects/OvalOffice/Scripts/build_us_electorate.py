"""Build reproducible county statistics and Albers-USA geometry from cached public data.
See Content/FourYears/Electorate/README.md for vintages, joins and model limitations.
"""
from pathlib import Path
import sys,json,csv,math,collections,hashlib,zipfile,re,unicodedata
P=Path(__file__).resolve().parents[1];sys.path.insert(0,str(P/'Saved/AtlasTools'))
import numpy as np, shapefile, mapbox_earcut as earcut
from shapely.geometry import shape,Polygon,MultiPolygon,Point
from shapely.ops import transform
from pyproj import Transformer
from PIL import Image,ImageDraw,ImageFont
SRC=P/'Saved/ElectorateSources';OUT=P/'Content/FourYears/Electorate';OUT.mkdir(parents=True,exist_ok=True)
def rows(name,encoding='utf-8-sig'):return list(csv.DictReader((SRC/name).open(encoding=encoding,newline='')))
def ers(kind):return {a['FIPSTXT']:a for i in [0,2000] for a in [r['attributes'] for r in json.loads((SRC/f'{kind}-{i}.json').read_text())['features']]}
people,income,jobs,classification=[ers(k) for k in ['People','Income','Jobs','County_Classifications']]
for k in ['states','counties','cities']:
 with zipfile.ZipFile(SRC/(k+'.zip')) as z:
  folder=(SRC/k).resolve()
  for member in z.infolist():
   if not (folder/member.filename).resolve().is_relative_to(folder):raise ValueError('Unsafe archive path')
  z.extractall(folder)
def read(k):return shapefile.Reader(str(next((SRC/k).glob('*.shp'))),encoding='utf-8')
def clean(s):return s.strip().replace('New Netherlands','New Netherland')
woodard={r['GEOID'].zfill(5):clean(r['AN_TITLE']) for r in rows('woodard-counties.csv','cp1252')}
# County-equivalent changes since the author's published 2017 table; Kalawao is in Hawaii.
for new,old in [('02158','02270'),('46102','46113'),('02063','02261'),('02066','02261')]:woodard[new]=woodard[old]
woodard['15005']='Greater Polynesia';woodard['01059']='Greater Appalachia'
names=['Yankeedom','New Netherland','Midlands','Tidewater','Greater Appalachia','Deep South','New France','El Norte','Left Coast','Far West','First Nation','Spanish Caribbean','Greater Polynesia','Federal Entity']
ids=['yankeedom','new_netherland','midlands','tidewater','appalachia','deep_south','new_france','el_norte','left_coast','far_west','first_nation','spanish_caribbean','polynesia','federal']
colors=['71b6c9','987bc1','d9bf70','71a791','d79254','bf6578','b78dcb','d4a574','6dbda3','859cca','ad8e70','da91b1','8abfac','c4ccdc']
# Authored game responses, not survey estimates or claims about every resident.
# service: public investment affinity; autonomy: local discretion; ecology: environmental salience.
profiles=[(.8,.0,.6),(.5,.3,.4),(.25,.6,.1),(.4,.3,.2),(-.3,1.,-.2),(-.4,.5,-.2),(.15,.5,.2),(.45,.55,.25),(.8,.6,1.),(-.25,.9,.15),(.4,1.,.8),(.15,.8,.15),(.6,.6,.85),(.6,.25,.45)]
blurbs=['Public institutions, education and civic accountability.','Commerce, pluralism and metropolitan public services.','Pragmatic public benefits with local discretion.','Institutional continuity and reliable public administration.','Local autonomy, household security and economic independence.','Private enterprise, community institutions and public order.','Community continuity, livelihoods and local public services.','Cross-border commerce, family livelihoods and public investment.','Environmental stewardship, civil liberties and public services.','Resource livelihoods, infrastructure and local control.','Community self-government, land stewardship and service access.','International commerce, household opportunity and local autonomy.','Island livelihoods, community services and environmental resilience.','Federal employment, public institutions and civic accountability.']
cultures=[{'id':i,'name':n,'color':c,'service':v[0],'autonomy':v[1],'ecology':v[2],'description':b} for i,n,c,v,b in zip(ids,names,colors,profiles,blurbs)]
name_id=dict(zip(names,ids))
ev=dict(zip('AL AK AZ AR CA CO CT DE DC FL GA HI ID IL IN IA KS KY LA ME MD MA MI MN MS MO MT NE NV NH NJ NM NY NC ND OH OK OR PA RI SC SD TN TX UT VT VA WA WV WI WY'.split(),[9,3,11,6,54,10,7,3,3,30,16,4,4,19,11,6,6,8,8,4,10,11,15,10,6,10,4,5,6,4,14,5,28,16,3,17,7,8,19,4,9,3,11,40,6,3,13,12,4,10,3]))
assert sum(ev.values())==538
state_raw={r.record.as_dict()['STUSPS']:r for r in read('states').iterShapeRecords() if r.record.as_dict()['STUSPS'] in ev}
states=[]
sr=rows('elections-state-2024.csv')
for ab,r in sorted(state_raw.items()):
 a=r.record.as_dict();votes=collections.defaultdict(float)
 for e in sr:
  if e['state_po']==ab and e['mode'] in ['TOTAL','NA']:votes[e['party_simplified']]+=float(e['votes'])
 d= votes['DEMOCRAT'];rep=votes['REPUBLICAN'];assert d>0 and rep>0
 states.append({'id':ab,'fips':a['STATEFP'],'name':a['NAME'],'ev':ev[ab],'demVotes':d,'repVotes':rep,'dem2024':d/(d+rep)*100,'total2024':sum(votes.values())})
state_by={s['id']:s for s in states}
# Prefer supplied totals to vote-mode subtotals, never add both together.
groups=collections.defaultdict(list)
def normalize(name):
 name=unicodedata.normalize('NFKD',name).encode('ascii','ignore').decode().upper()
 return re.sub('[^A-Z]','',re.sub(r'\b(COUNTY|PARISH|BOROUGH|CENSUS AREA)\b','',name)).replace('SAINT','ST')
county_lookup={(r.as_dict()['STUSPS'],normalize(r.as_dict()['NAME'])):r.as_dict()['GEOID'] for r in read('counties').records()}
county_identity={r.as_dict()['GEOID']:(r.as_dict()['STUSPS'],normalize(r.as_dict()['NAME'])) for r in read('counties').records()}
fixes={'SHANNON':'OGLALA LAKOTA','LA PAZ':'LA PAZ','MCDUFFIE':'MCDUFFIE'}
join_fixes=[]
for r in rows('elections.csv'):
 if r['year'] not in ['2020','2024'] or r['party'] not in ['DEMOCRAT','REPUBLICAN'] or not r['county_fips'] or r['state_po']=='AK':continue
 f=f"{int(float(r['county_fips'])):05d}"
 original=f
 matched=county_lookup.get((r['state_po'],normalize(fixes.get(r['county_name'],r['county_name']))))
 if county_identity.get(f)==(r['state_po'],normalize(r['county_name'])):pass
 elif matched:f=matched
 elif r['state_po']=='RI' and len(f)==10:f=f[:5] # published municipality GEOID includes county prefix
 if original!=f:join_fixes.append((r['year'],r['state_po'],r['county_name'],original,f))
 groups[(r['year'],f,r['candidate'],r['party'])].append(r)
returns=collections.defaultdict(lambda:collections.defaultdict(float))
for (year,f,candidate,party),items in groups.items():
 totals=[r for r in items if r['mode'] in ['TOTAL','TOTAL VOTES']]
 chosen=totals if totals else items
 returns[(year,f)][party]+=sum(float(r['candidatevotes']) for r in chosen)
FWD=Transformer.from_crs('EPSG:4326','EPSG:5070',always_xy=True)
AK=Transformer.from_crs('EPSG:4326','EPSG:3338',always_xy=True)
def project(lon,lat,ab):
 if ab=='AK':
  x,y=AK.transform(lon,lat);return .045+(x+2200000)/3700000*.265,.74+(2400000-y)/2700000*.245
 if ab=='HI':
  # Northwestern islands occupy a separate compressed strip above the main Hawaiian inset.
  if lon < -160.5:return .32+(lon+179)/18.5*.16,.745+(29-lat)/10*.035
  return .32+(lon+160.5)/6*.16,.79+(22.6-lat)/4*.15
 x,y=FWD.transform(lon,lat);return .025+(x+2350000)/4650000*.95,.035+(3200000-y)/3250000*.84
def pg(g,ab):
 def fn(x,y,z=None):
  if np.ndim(x)==0:return project(x,y,ab)
  pairs=[project(a,b,ab) for a,b in zip(x,y)];return tuple(zip(*pairs))
 return transform(fn,g)
def polygons(g,tolerance=.000065):
 g=g.simplify(tolerance,preserve_topology=True)
 if not g.is_valid:g=g.buffer(0)
 out=[]
 for poly in (g.geoms if isinstance(g,MultiPolygon) else [g]):
  if not isinstance(poly,Polygon) or poly.area<1e-10:continue
  rings=[np.asarray(poly.exterior.coords)[:-1]]+[np.asarray(r.coords)[:-1] for r in poly.interiors]
  verts=np.vstack(rings).astype(np.float64);idx=earcut.triangulate_float64(verts,np.cumsum([len(r) for r in rings],dtype=np.uint32))
  assert len(verts)<65535
  out.append({'rings':[np.round(r,7).tolist() for r in rings],'v':np.round(verts,7).tolist(),'i':idx.tolist(),'bounds':list(poly.bounds)})
 return out
counties=[];geo=[];fallback=[]
def n(r,k,default):
 v=r.get(k);return float(v) if isinstance(v,(int,float)) and math.isfinite(v) and v>=0 else default
for record in read('counties').iterShapeRecords():
 a=record.record.as_dict();ab=a['STUSPS'];f=a['GEOID']
 if ab not in ev:continue
 d=people[f];inc=income[f];j=jobs[f];cl=classification.get(f,{})
 pop=n(d,'POPESTIMATE2021',0);adult=pop*(1-n(d,'Under18Pct2020',22)/100);senior=min(.65,n(d,'Age65AndOlderPct2020',17)*pop/max(1,adult)/100)
 culture=woodard[f];mix={'Yankeedom-Midlands':{'yankeedom':.5,'midlands':.5},'New France-Deep South':{'new_france':.5,'deep_south':.5}}.get(culture)
 if mix is None:mix={name_id[culture]:1.}
 dominant=max(mix,key=mix.get)
 v=returns.get(('2024',f),{});v20=returns.get(('2020',f),{})
 measured=v.get('DEMOCRAT',0)>0 and v.get('REPUBLICAN',0)>0
 baseline=v.get('DEMOCRAT',0)/max(1,sum(v.values()))*100 if measured else state_by[ab]['dem2024']
 total=sum(v.values()) if measured else adult*.6
 if not measured:fallback.append(f)
 region='coast' if ab in 'WA OR CA HI NY NJ CT MA RI NH VT ME MD DE DC'.split() else 'lakes' if ab in 'PA OH MI IN IL WI MN IA MO'.split() else 'plains' if ab in 'AK MT ID WY ND SD NE KS CO UT NV'.split() else 'sunbelt'
 c={'id':f,'name':a['NAME'],'fullName':a['NAMELSAD'],'state':ab,'region':region,'population':round(pop),'adults':round(adult),'senior':round(senior,5),'college':n(d,'Ed5CollegePlusPct',30)/100,'homeowner':n(d,'OwnHomePct',64)/100,'income':n(inc,'Median_HH_Inc_ACS',60000),'poverty':n(inc,'Poverty_Rate_ACS',13),'density':n(d,'PopDensity2020',0),'ruralCode':n(cl,'RuralUrbanContinuumCode2013',7),'unemployment':n(j,'UnempRate2021',5),'manufacturing':n(j,'PctEmpManufacturing',10)/100,'mining':n(j,'PctEmpMining',1)/100,'farming':n(j,'PctEmpAgriculture',2)/100,'culture':dominant,'cultures':mix,'dem2024':baseline,'dem2020':v20.get('DEMOCRAT',0)/max(1,sum(v20.values()))*100 if v20 else baseline,'ballots':total,'electionSource':'county' if measured else 'state estimate','demographics':{'white':n(d,'WhiteNonHispanicPct2020',0),'black':n(d,'BlackNonHispanicPct2020',0),'asian':n(d,'AsianNonHispanicPct2020',0),'native':n(d,'NativeAmericanNonHispanicPct2020',0),'hispanic':n(d,'HispanicPct2020',0)}}
 g=shape(record.shape.__geo_interface__);center=g.representative_point();c['lon']=round(center.x,4);c['lat']=round(center.y,4)
 geom=pg(g,ab);polys=polygons(geom);main=max(polys,key=lambda p:(p['bounds'][2]-p['bounds'][0])*(p['bounds'][3]-p['bounds'][1]))
 geo.append({'id':f,'state':ab,'name':a['NAME'],'label':list(geom.representative_point().coords)[0],'focus':list(geom.bounds),'polygons':polys});counties.append(c)
counties.sort(key=lambda c:c['id']);geo.sort(key=lambda c:c['id'])
# State certification anchors resolve reporting discrepancies and omitted county-equivalents.
# Preserve the original share separately so calibrated inputs are never presented as raw returns.
calibration=[]
for s in states:
 cs=[c for c in counties if c['state']==s['id']]
 dem=sum(c['ballots']*c['dem2024']/100 for c in cs);rep=sum(c['ballots']*(1-c['dem2024']/100) for c in cs)
 ds=s['demVotes']/dem;rs=s['repVotes']/rep
 calibration.append({'state':s['id'],'demFactor':ds,'repFactor':rs})
 for c in cs:
  d=c['ballots']*c['dem2024']/100*ds;r=c['ballots']*(1-c['dem2024']/100)*rs
  c['rawDem2024']=round(c['dem2024'],4);c['dem2024']=round(100*d/(d+r),6);c['ballots']=round(d+r,3)
  c['baseTurnout']=round(max(.25,min(.95,c['ballots']/max(1,c['adults']))),6)
  # Stable local issue weights are measured demographic inputs interpreted by authored coefficients.
  rural=(c['ruralCode']-1)/8;low=max(0,min(1,(85000-c['income'])/60000))
  service=sum(next(p['service'] for p in cultures if p['id']==k)*w for k,w in c['cultures'].items())
  ecology=sum(next(p['ecology'] for p in cultures if p['id']==k)*w for k,w in c['cultures'].items())
  c['weights']={'prices':1.5+low,'jobs':1.2+3*c['manufacturing']+c['unemployment']/10,'growth':1.,'health':1.+2*c['senior']+low*.5,'schools':.8+c['college']+service*.25,'housing':.7+(1-c['homeowner'])*2+(1-rural)*.5,'safety':.8+rural*.3,'climate':max(.3,.65+ecology*.6+c['college']*.5),'energy':.8+rural*.5+c['mining']*3,'equality':max(.3,.7+low*.5+service*.4),'liberty':.9,'trust':1.1}
  c['weights']={k:round(v,5) for k,v in c['weights'].items()}
districts=[('ME','ME-1',258863,165214),('ME','ME-2',176789,212763),('NE','NE-1',136153,177666),('NE','NE-2',163541,148905),('NE','NE-3',70301,238245)]
districts=[{'id':i,'state':s,'dem2024':100*d/(d+r),'ev':1} for s,i,d,r in districts]
data={'version':1,'sourceYears':{'demographics':'2020 Census / 2017-2021 ACS / 2021 population','elections':'2024 (2020 comparison)','cultures':'Woodard county table published 2017; county-code updates'},'cultures':cultures,'states':states,'districts':districts,'counties':counties}
(P/'Prototype/data/us-electorate.json').write_text(json.dumps(data,separators=(',',':')),encoding='utf-8')
stategeo=[]
for s in states:
 g=pg(shape(state_raw[s['id']].shape.__geo_interface__),s['id']);stategeo.append({'id':s['id'],'name':s['name'],'label':list(g.representative_point().coords)[0],'focus':list(g.bounds),'polygons':polygons(g)})
# Named city symbols only, sourced from the existing Natural Earth city dataset.
cities=[]
cityfile=next((SRC/'cities').rglob('*.shp'))
for r in shapefile.Reader(str(cityfile),encoding='utf-8').iterShapeRecords():
 a=r.record.as_dict()
 if a['adm0_a3']!='USA' or a['scalerank']>6:continue
 lon,lat=a['longitude'],a['latitude'];ab='AK' if lat>51 else 'HI' if lon<-150 else ''
 cities.append({'name':a['name'],'p':list(project(lon,lat,ab)),'rank':a['scalerank'],'capital':bool(a['adm0cap'])})
atlas={'aspect':1.58,'counties':geo,'states':stategeo,'cities':cities}
(OUT/'us.json').write_text(json.dumps(atlas,separators=(',',':')),encoding='utf-8')
manifest={'counties':len(counties),'states':len(states),'electors':sum(ev.values()),'cities':len(cities),'fallbackElectionCounties':fallback,'correctedElectionJoins':sorted(set(join_fixes)),'calibration':calibration,'sourceHashes':json.loads((SRC/'manifest.json').read_text())}
(OUT/'build.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
# A review image of the exact game geometry; the live map renders vector meshes.
W,H=2400,1520;im=Image.new('RGB',(W,H),'#101f30');draw=ImageDraw.Draw(im);palette={x['id']:'#'+x['color'] for x in cultures};by={c['id']:c for c in counties}
for g in geo:
 for p in g['polygons']:
  draw.polygon([(x*W,y*H) for x,y in p['rings'][0]],fill=palette[by[g['id']]['culture']],outline='#526373')
  for ring in p['rings'][1:]:draw.polygon([(x*W,y*H) for x,y in ring],fill='#101f30')
for g in stategeo:
 for p in g['polygons']:
  for ring in p['rings']:draw.line([(x*W,y*H) for x,y in ring]+[(ring[0][0]*W,ring[0][1]*H)],fill='#eff2ec',width=2)
im.save(OUT/'US_Cultural_Atlas.png')
print(json.dumps({k:manifest[k] for k in ['counties','states','electors','cities','fallbackElectionCounties']}),flush=True)
