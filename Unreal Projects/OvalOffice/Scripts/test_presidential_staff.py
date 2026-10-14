"""PIE smoke test for the imported cast, room travel, and existing NPC conversations.
Does not make campaign choices or write a game save. Ends PIE after the check.
"""
import unreal as u,json,time,traceback,hashlib
from pathlib import Path
P=Path(u.Paths.project_dir());editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
save=P/'Saved/FourYears/Term.json';original=save.read_bytes();checks=[];stage=0;last=0;start=time.monotonic()
def check(label,condition):
 checks.append(dict(check=label,passed=bool(condition)))
 if not condition:raise AssertionError(label)
def finish(status,error=''):
 u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
 report=dict(status=status,error=error,checks=checks,save_unchanged=save.read_bytes()==original)
 (P/'Saved/StaffValidation.json').write_text(json.dumps(report,indent=2));u.unregister_slate_post_tick_callback(staff_test_handle)
def validate_cast(w,place,minimum):
 staff=[a for a in u.GameplayStatics.get_all_actors_of_class(w,u.FourYearsStaff) if a.place==place]
 check(place+' populated',len(staff)>=minimum)
 for a in staff:
  comps=a.get_components_by_class(u.StaticMeshComponent)
  check(a.person_name+' has two authored mesh parts',len(comps)==2)
  for comp in comps:
   mesh=comp.static_mesh;check(a.person_name+' detailed mesh',mesh and '/PresidentialStaff/' in mesh.get_path_name())
   check(a.person_name+' materials assigned',all(comp.get_material(i) for i in range(comp.get_num_materials())))
 return staff
def tick(dt):
 global stage,last
 try:
  if time.monotonic()-last<1.2:return
  last=time.monotonic()
  if last-start>300:raise RuntimeError('NPC travel validation timed out')
  w=editor.get_game_world()
  if not w:return
  pc=u.GameplayStatics.get_player_controller(w,0);pawn=u.GameplayStatics.get_player_pawn(w,0)
  if not pc or not pawn:return
  if stage==0:
   for c in json.loads((P/'SourceAssets/PresidentialStaff/manifest.json').read_text())['characters']:
    for part in ['Head','Standing','Seated']:check(c['id']+' '+part+' imported',bool(u.load_asset('/Game/PresidentialStaff/Meshes/SM_'+c['id']+'_'+part)))
   validate_cast(w,'oval',3);validate_cast(w,'residence',2);validate_cast(w,'limo',1)
   pc.close_screen();npc=next(a for a in u.GameplayStatics.get_all_actors_of_class(w,u.FourYearsStaff) if a.person_name=='Maya Chen' and a.place=='oval')
   pos=npc.get_actor_location();pawn.set_actor_location(u.Vector(pos.x-80,pos.y,165),False,True)
   check('Named conversation prompt retained','Maya Chen' in pc.get_interaction_prompt());pc.interact();stage=1
  elif stage==1:
   check('Adviser conversation opens',pc.is_screen_open());pc.navigate_presidency('travel:cabinet');stage=2
  elif stage==2:
   if 'CabinetRoom' not in w.get_name():return
   validate_cast(w,'cabinet',6);check('Cabinet perimeter clear',pawn.can_stand(u.Vector2D(0,280)))
   scene=u.GameplayStatics.get_all_actors_of_class(w,u.StaticMeshActor)
   check('Old cartoon garden removed',not any(a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name()=='SM_Rose_Garden' for a in scene))
   photos=[a for a in scene if a.static_mesh_component.get_material(0) and a.static_mesh_component.get_material(0).get_name()=='M_PhotographicGarden']
   check('One continuous photographic garden',len(photos)==1)
   check('Garden never blocks walking',not photos[0].get_actor_enable_collision())
   for identity in ['Ruiz','Bell']:
    check(identity+' dress material loaded',bool(u.load_asset('/Game/PresidentialStaff/Materials/ST_'+identity+'_suit')))
   pc.navigate_presidency('travel:aircraft');stage=3
  elif stage==3:
   if 'AirForceOne' not in w.get_name():return
   validate_cast(w,'aircraft',1);check('Aircraft aisle clear',pawn.can_stand(u.Vector2D(800,276)));pc.navigate_presidency('travel:oval');stage=4
  elif stage==4:
   if 'OvalOffice' not in w.get_name():return
   check('Existing save unchanged',save.read_bytes()==original);finish('passed')
 except Exception:finish('failed',traceback.format_exc())
staff_test_handle=u.register_slate_post_tick_callback(tick)
