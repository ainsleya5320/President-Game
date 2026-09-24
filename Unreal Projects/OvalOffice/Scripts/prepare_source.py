from pathlib import Path
import json
P=Path(__file__).resolve().parents[1]/'SourceAssets'
d=json.loads((P/'manifest.json').read_text());lines=[]
for m in d['materials']:
 lines.extend(['newmtl '+m['id'],'Kd '+' '.join(str(x) for x in m['color']),'Ks 0.2 0.2 0.2','Ns 32','d 1',''])
(P/'materials.mtl').write_text('\n'.join(lines))
for g in d['groups']:
 p=P/'Meshes'/(g['name']+'.obj');s=p.read_text()
 if not s.startswith('mtllib'):p.write_text('mtllib ../materials.mtl\n'+s)
print('Prepared',len(d['groups']),'mesh groups')
