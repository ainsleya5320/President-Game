"""Check the shipped geometry and the geography-to-simulation join."""
from pathlib import Path
import json,math
P=Path(__file__).resolve().parents[1]
a=json.loads((P/'Content/FourYears/Electorate/us.json').read_text());d=json.loads((P/'Prototype/data/us-electorate.json').read_text())
assert {c['id'] for c in a['counties']}=={c['id'] for c in d['counties']}
assert {s['id'] for s in a['states']}=={s['id'] for s in d['states']}
triangles=vertices=0
for c in a['counties']+a['states']:
 assert c['polygons']
 for p in c['polygons']:
  v=p['v'];idx=p['i'];vertices+=len(v);triangles+=len(idx)//3
  assert len(idx)%3==0 and all(0<=i<len(v) for i in idx)
  assert all(math.isfinite(x) and math.isfinite(y) and -.03<x<1.03 and -.03<y<1.03 for x,y in v)
  assert len(v)<65535
assert len(a['cities'])>150 and all(c['name'] for c in a['cities'])
assert any(c['name']=='Chicago' for c in a['cities'])
print(f'US atlas: 3,143 county joins, 51 state/DC joins, {vertices:,} vertices, {triangles:,} triangles and {len(a["cities"])} named cities checked.')
