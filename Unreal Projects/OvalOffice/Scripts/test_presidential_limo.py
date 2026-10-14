"""Native PIE check: detailed limo, safe aisle, phone, adviser, return and save."""
import unreal as u,json,time,traceback
from pathlib import Path
P=Path(u.Paths.project_dir());editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
save=P/'Saved/FourYears/Term.json';original=save.read_bytes();checks=[];stage=0;last=0;start=time.monotonic()
def check(label,condition):
 checks.append(dict(check=label,passed=bool(condition)))
 if not condition:raise AssertionError(label)
def finish(status,error=''):
 u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
 (P/'Saved/LimoValidation.json').write_text(json.dumps(dict(status=status,error=error,checks=checks,save_unchanged=save.read_bytes()==original),indent=2))
 u.unregister_slate_post_tick_callback(limo_test_handle)
def tick(dt):
 global stage,last
 try:
  if time.monotonic()-last<1.2:return
  last=time.monotonic()
  if last-start>180:raise RuntimeError('Limo validation timed out')
  w=editor.get_game_world()
  if not w:return
  pc=u.GameplayStatics.get_player_controller(w,0);pawn=u.GameplayStatics.get_player_pawn(w,0)
  if not pc or not pawn:return
  if stage==0:
   pc.close_screen();pc.navigate_presidency('travel:limo');stage=1
  elif stage==1:
   presence=u.GameplayStatics.get_all_actors_of_class(w,u.FourYearsPresence)[0]
   cabin=[c for c in presence.get_components_by_class(u.StaticMeshComponent) if c.static_mesh and '/PresidentialLimo/' in c.static_mesh.get_path_name()]
   check('Authored Blender cabin replaces placeholders',len(cabin)==1)
   check('Cabin has all assigned finishes',all(cabin[0].get_material(i) for i in range(cabin[0].get_num_materials())))
   check('Leather, wood, trim and displays',cabin[0].get_num_materials()>=12)
   check('Arrives inside motorcade',4760<pawn.get_actor_location().x<5145)
   for x in [4800,4860,4920,4980,5040,5100,5140]:check('Clear aisle '+str(x),pawn.can_stand(u.Vector2D(x,20)))
   check('Cabinet tray protected',not pawn.can_stand(u.Vector2D(4950,-40)))
   check('Rear bench protected',not pawn.can_stand(u.Vector2D(5200,0)))
   pawn.set_actor_location(u.Vector(5080,-10,145),False,True)
   check('Secure phone reachable','secure phone' in pc.get_interaction_prompt().lower())
   pc.interact();stage=2
  elif stage==2:
   check('Phone opens congressional relations',pc.is_screen_open());pc.close_screen()
   pawn.set_actor_location(u.Vector(5140,45,145),False,True)
   check('Maya conversation reachable','Maya Chen' in pc.get_interaction_prompt());pc.interact();stage=3
  elif stage==3:
   check('Adviser conversation opens',pc.is_screen_open());pc.close_screen();pc.navigate_presidency('travel:oval');stage=4
  elif stage==4:
   check('Returns to Oval Office',pawn.get_actor_location().x<1000)
   check('Campaign save unchanged',save.read_bytes()==original);finish('passed')
 except Exception:finish('failed',traceback.format_exc())
limo_test_handle=u.register_slate_post_tick_callback(tick)
