from pathlib import Path
import json,html
P=Path(__file__).parent
r=json.loads((P/'walkability_report.json').read_text());solids=json.loads((P/'collision_footprints.json').read_text())
assert r['all_routes_pass']
def xy(p):return (55+(p[0]+3.6)*53,135+p[1]*53)
def points(poly):return ' '.join('%.2f,%.2f'%xy(p) for p in poly)
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="820" height="900" viewBox="0 0 820 900"><rect width="820" height="900" fill="#10212c"/><style>text{font-family:Arial,sans-serif;fill:#e8e5d8}.small{font-size:13px;fill:#afbfca}.label{font-size:16px;font-weight:bold}</style>',
'<text x="48" y="44" font-size="12" letter-spacing="3" fill="#d5b676">AIR FORCE ONE / WALKING CLEARANCE</text>',
'<text x="48" y="85" font-size="29">Room to move</text>',
'<text class="small" x="48" y="110">Top-down plan of the revised Blender geometry</text>']
svg.append('<rect x="55" y="135" width="382" height="688" rx="2" fill="#b8ab8e"/>')
for item in solids:
 svg.append('<polygon points="'+points(item['polygon'])+'" fill="#ded5bd" stroke="#786f59" stroke-width=".5"/>')
for name,path in r['paths'].items():
 svg.append('<polyline points="'+points(path)+'" fill="none" stroke="#2786ac" stroke-width="4" stroke-linecap="round" stroke-linejoin="round"/>')
 x,y=xy(path[-1]);svg.append(f'<circle cx="{x}" cy="{y}" r="6" fill="#155d7b" stroke="#e3f5ed" stroke-width="2"/>')
for label,x,y in [('OFFICE',-1,3.6),('CONFERENCE',-.6,6.1),('HALL',2.8,11.8)]:
 x,y=xy((x,y));svg.append(f'<text x="{x}" y="{y}" font-size="12" text-anchor="middle" style="fill:#283f47;font-weight:bold">{label}</text>')
lines=[('All 7 routes connected',165,'label'),('Clear access from the hallway to:',195,'small'),('• the office and lounge',220,'small'),('• the desk-side aisle',243,'small'),('• both sides of the conference table',266,'small'),('• the far end of the conference room',289,'small'),('Test player',349,'label'),('1.80 m tall · 0.68 m wide',378,'small'),('Plus 2 cm clearance on each side',401,'small'),('Door openings',451,'label'),(f"{r['door_clear_width_m']:.2f} m clear",480,'small'),('Hallway between handrails',530,'label'),(f"{r['hall_clear_width_m']:.2f} m clear",559,'small'),('Furniture stays human-sized.',625,'small'),('The cabin shell and passages',648,'small'),('were widened to make space.',671,'small'),('Geometry checked in Blender.',737,'small'),('Native Unreal collision and',760,'small'),('walking routes tested separately.',783,'small')]
for label,y,cl in lines:svg.append(f'<text x="473" y="{y}" class="{cl}">{html.escape(label)}</text>')
svg.append('<text class="small" x="55" y="859">Blue routes: tested player-centre paths. Cream shapes: conservative solid footprints.</text></svg>')
(P/'Walking_Clearance.svg').write_text('\n'.join(svg),encoding='utf-8')
print('Clearance plan created')
