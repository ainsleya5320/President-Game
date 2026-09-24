from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from PIL import Image
import json,struct
P=Path(__file__).parent
renders=[P/'renders'/('Air_Force_One_'+name+'.png') for name in ['Office','Conference','Hallway','Cutaway']]
for p in renders:
 with Image.open(p) as im:
  im.verify()
 with Image.open(p) as im:
  assert im.size==(1500,1000),(p,im.size)
data=(P/'Air_Force_One.glb').read_bytes()
magic,version,total=struct.unpack_from('<III',data)
assert magic==0x46546c67 and version==2 and total==len(data)
n=struct.unpack_from('<I',data,12)[0];doc=json.loads(data[20:20+n])
assert all('bufferView' in im for im in doc['images'])
nodes={node.get('name','') for node in doc['nodes']}
assert all(name in nodes for name in ['PLAYER_START_OFFICE','PLAYER_START_HALL','PLAYER_START_CONFERENCE'])
validation={'renders_checked':4,'render_resolution':[1500,1000],'glb_meshes':len(doc['meshes']),'embedded_material_images':len(doc['images']),'named_player_starts':3,'portable_model_has_external_image_dependencies':False}
walk=json.loads((P/'walkability_report.json').read_text())
assert walk['all_routes_pass'] and all(walk['player_starts_clear_and_connected'].values())
validation['walking_clearance']={key:walk[key] for key in ['player_width_m','player_height_m','hall_clear_width_m','door_clear_width_m','all_routes_pass']}
(P/'validation.json').write_text(json.dumps(validation,indent=2))
files=[P/name for name in ['Air_Force_One.blend','Air_Force_One.glb','README.md','index.html','level_manifest.json','validation.json','build_air_force_one.py','make_materials.py','render_and_export.py','finalize_scene.py','ensure_walkable.py','walkability_report.json','collision_footprints.json','Walking_Clearance.svg']]+renders+list((P/'textures').glob('*'))+list((P/'references').glob('*'))
files.append(P/'draw_clearance.py')
with ZipFile(P/'Air_Force_One_Package.zip','w',ZIP_DEFLATED,compresslevel=6) as archive:
 for p in files:archive.write(p,p.relative_to(P))
print(json.dumps(validation,indent=2));print('PACKAGE_READY', (P/'Air_Force_One_Package.zip').stat().st_size)
