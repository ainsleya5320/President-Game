from pathlib import Path
import urllib.request, zipfile, concurrent.futures, hashlib, json
P=Path(__file__).resolve().parents[1]
cache=P/'Saved/AtlasSources';cache.mkdir(parents=True,exist_ok=True)
sources={
 'countries':'https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_0_countries.zip',
 'cities':'https://naciscdn.org/naturalearth/10m/cultural/ne_10m_populated_places_simple.zip',
 'relief':'https://naciscdn.org/naturalearth/50m/raster/NE2_50M_SR.zip',
 'lakes':'https://naciscdn.org/naturalearth/50m/physical/ne_50m_lakes.zip',
 'rivers':'https://naciscdn.org/naturalearth/50m/physical/ne_50m_rivers_lake_centerlines.zip'}
def fetch(item):
 name,url=item;dst=cache/(name+'.zip');folder=cache/name
 if not dst.exists():
  with urllib.request.urlopen(url,timeout=120) as response, dst.open('wb') as out:
   while chunk:=response.read(1024*1024):out.write(chunk)
 folder.mkdir(exist_ok=True)
 with zipfile.ZipFile(dst) as z:
  for member in z.infolist():
   target=(folder/member.filename).resolve()
   if not target.is_relative_to(folder.resolve()):raise ValueError('Unsafe archive path')
  z.extractall(folder)
 return {'name':name,'url':url,'sha256':hashlib.sha256(dst.read_bytes()).hexdigest(),'bytes':dst.stat().st_size}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
 results=list(pool.map(fetch,sources.items()))
(cache/'sources.json').write_text(json.dumps(results,indent=2))
print(json.dumps(results,indent=2))
