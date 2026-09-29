"""Cache public primary-source inputs for the offline US electorate build."""
from pathlib import Path
import urllib.request, urllib.parse, urllib.error, json, hashlib, concurrent.futures
P=Path(__file__).resolve().parents[1]
OUT=P/'Saved/ElectorateSources'; OUT.mkdir(parents=True,exist_ok=True)
def fetch(name,url):
 path=OUT/name
 if not path.exists():
  req=urllib.request.Request(url,headers={'User-Agent':'FourYearsResearch/1.0'})
  try:
   with urllib.request.urlopen(req,timeout=120) as response:path.write_bytes(response.read())
  except urllib.error.HTTPError as e:
   print(name,e.code,e.read()[:500],flush=True);raise
 return {'file':name,'url':url,'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
if __name__=='__main__':
 jobs=[('cities.zip','https://naciscdn.org/naturalearth/10m/cultural/ne_10m_populated_places_simple.zip'),
       ('elections.csv','https://raw.githubusercontent.com/9t8/cmsc320_project/master/countypres_2000-2024.csv'),
       ('woodard-counties.csv','https://specialprojects.pressherald.com/americannations/county_results.csv'),
       ('elections-state-2024.csv','https://raw.githubusercontent.com/MEDSL/2024-elections-official/main/2024-president-state.csv'),
       ('woodard-2017.html','https://www.centralmaine.com/2017/01/06/the-american-nations-in-the-2016-presidential-election/'),
       ('counties.zip','https://www2.census.gov/geo/tiger/GENZ2021/shp/cb_2021_us_county_500k.zip'),
       ('states.zip','https://www2.census.gov/geo/tiger/GENZ2021/shp/cb_2021_us_state_500k.zip'),
       ('woodard-state-shares.csv','https://datawrapper.dwcdn.net/2Zc9P/3/dataset.csv')]
 for kind in ['People','Income','Jobs','County_Classifications']:
  jobs.append((kind+'-metadata.json',f'https://gisportal.ers.usda.gov/server/rest/services/Rural_Atlas_Data/{kind}/MapServer/0?f=pjson'))
  for offset in [0,2000]:
   q=urllib.parse.urlencode({'where':'1=1','outFields':'*','returnGeometry':'false','resultOffset':offset,'resultRecordCount':2000,'orderByFields':'OBJECTID','f':'json'})
   jobs.append((f'{kind}-{offset}.json',f'https://gisportal.ers.usda.gov/server/rest/services/Rural_Atlas_Data/{kind}/MapServer/0/query?{q}'))
 records=[]
 with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
  for future in concurrent.futures.as_completed([pool.submit(fetch,*p) for p in jobs]):
   try:result=future.result();records.append(result);print(result['file'],result['bytes'],flush=True)
   except Exception as e:print(str(e),flush=True)
 (OUT/'manifest.json').write_text(json.dumps(records,indent=2))
