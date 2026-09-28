"""Download museum-designated public-domain images, preserving provenance."""
from pathlib import Path
import urllib.request,json,concurrent.futures
P=Path(__file__).resolve().parents[1]/'SourceAssets/DecorRefresh'
(P/'Textures').mkdir(parents=True,exist_ok=True)
IDS=[3294,146701,90048,68792,120163,64715,57163,68388,30701,129639,100489,94127,14309,81564,152747,71971,121377,23119,59455,151108,84088,72398,49356]
def fetch(id):
 d=json.load(urllib.request.urlopen(f'https://api.artic.edu/api/v1/artworks/{id}',timeout=30))['data']
 assert d['is_public_domain'] and d['image_id']
 filename=f'art_{id}.jpg';url=f"https://www.artic.edu/iiif/2/{d['image_id']}/full/843,/0/default.jpg"
 if not (P/'Textures'/filename).exists():
  req=urllib.request.Request(url,headers={'User-Agent':'Mozilla/5.0','Referer':'https://www.artic.edu/'})
  with urllib.request.urlopen(req,timeout=30) as r:(P/'Textures'/filename).write_bytes(r.read())
 return {'id':id,'title':d['title'],'artist':d['artist_title'],'file':filename,'public_domain':True,'source':f'https://www.artic.edu/artworks/{id}','image_url':url}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:art=list(pool.map(fetch,IDS))
(P/'art_provenance.json').write_text(json.dumps(art,indent=2))
print('Downloaded',len(art),'public-domain paintings')
