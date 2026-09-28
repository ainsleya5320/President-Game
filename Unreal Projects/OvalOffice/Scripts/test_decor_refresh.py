"""Read-only asset validation; run in Unreal outside Play before visual playtest."""
import unreal as u,json,hashlib
from pathlib import Path
P=Path(u.Paths.project_dir());S=P/'SourceAssets/DecorRefresh';D=json.loads((S/'manifest.json').read_text())
checks=[];save=P/'Saved/FourYears/Term.json';before=save.read_bytes()
def check(name,value):
 checks.append(dict(check=name,passed=bool(value)));assert value,name
for g in D['groups']:
 mesh=u.load_asset('/Game/DecorRefresh/Meshes/SM_'+g['name']);check('Mesh '+g['name'],bool(mesh))
 assigned={slot.material_interface.get_path_name().split('.')[0] for slot in mesh.static_materials if slot.material_interface}
 check('All materials resolve '+g['name'],assigned==set(g['materials'].values()))
 bounds=mesh.get_bounding_box();check('Geometry '+g['name'],bounds.max.x>bounds.min.x and bounds.max.y>bounds.min.y and bounds.max.z>bounds.min.z)
for info in D['materials']:
 check('Material '+info['id'],bool(u.load_asset('/Game/DecorRefresh/Materials/'+info['id'])))
 if info['texture']:check('Texture '+info['texture'],bool(u.load_asset('/Game/DecorRefresh/Textures/T_'+Path(info['texture']).stem)))
art=[p for p in D['placements'] if p['kind']=='painting']
check('Fifteen distinct aircraft paintings',len({p['title'] for p in art if p['scene']=='AirForceOne'})==15)
cab={p['title'] for p in art if p['scene']=='CabinetRoom'};air={p['title'] for p in art if p['scene']=='AirForceOne'}
check('Cabinet artwork differs from aircraft',len(cab)==4 and not cab.intersection(air))
check('Four draped flags',sum(p['kind']=='indoor_flag' for p in D['placements'])==4)
check('Three rebuilt telephones',sum(p['kind']=='telephone' for p in D['placements'])==3)
check('Eight photographed desk frames',sum(m['id'].startswith('DR_Memory_') for m in D['materials'])==8)
check('Saved presidency unchanged',save.read_bytes()==before)
(P/'Saved/DecorValidation.json').write_text(json.dumps(dict(status='passed',checks=checks,save_sha256=hashlib.sha256(before).hexdigest()),indent=2))
u.log('DECOR VALIDATION PASSED: '+str(len(checks))+' checks')
