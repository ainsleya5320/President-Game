# Developer validation: run while playing CabinetRoom. Never spends resources or changes a term.
import unreal as u, json, time, hashlib, traceback
from pathlib import Path
P=Path(u.Paths.project_dir());save=P/'Saved/FourYears/Term.json';initial=save.read_bytes();checks=[]
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
def state():
 w=editor.get_game_world()
 if not w:return None,None,None,None
 pc=u.GameplayStatics.get_player_controller(w,0);pawn=u.GameplayStatics.get_player_pawn(w,0);gi=u.GameplayStatics.get_game_instance(w)
 game=next((o for o in u.ObjectIterator(u.FourYearsSubsystem) if o.get_outer()==gi),None)
 return w,pc,pawn,game
def check(name,value):
 checks.append({'check':name,'passed':bool(value)})
 if not value:raise AssertionError(name)
def fingerprint(game):
 s=game.get_status();b=game.get_briefing()
 return (b.quarter,s.capital,s.appointments_left,s.decision_filed)
w,pc,pawn,game=state()
if not w or 'CabinetRoom' not in w.get_name():raise RuntimeError('Press Play in CabinetRoom before running this test')
initial_instance=u.GameplayStatics.get_game_instance(w);initial_state=fingerprint(game)
check('Native walker active',isinstance(pawn,u.FourYearsWalker))
for group in ['Hall_and_Doors','Windows_and_Drapery']:
 mesh=u.load_asset('/Game/CabinetRoom/Meshes/SM_'+group)
 check('Imported '+group+' matches positive-Y floor plan',mesh.get_bounding_box().min.y>0)
parent=u.load_asset('/Game/CabinetRoom/Materials/M_CabinetTextured')
check('Artwork material exposes its texture', 'Albedo' in [str(n) for n in u.MaterialEditingLibrary.get_texture_parameter_names(parent)])
# A continuous perimeter route, including the narrow section beside the sideboard.
route=[(-570,280),(570,280),(550,-280),(190,-280),(160,-243),(-160,-243),(-190,-280),(-570,-280),(-570,280),(-770,260),(-570,280)]
for j,(start,end) in enumerate(zip(route,route[1:])):
 for step in range(21):
  t=step/20;point=u.Vector2D(start[0]*(1-t)+end[0]*t,start[1]*(1-t)+end[1]*t)
  check('Walk route %d/%d'%(j,step),pawn.can_stand(point))
for name,point in [('table',(0,0)),('chair',(-200,170)),('wall',(640,0)),('outside hall',(-760,100)),('sideboard',(0,-330))]:check('Blocked '+name,not pawn.can_stand(u.Vector2D(*point)))
seats=[(-200,278,'Maya Chen'),(200,278,'Elena Ruiz'),(-200,-278,'Daniel Brooks'),(200,-278,'Samira Bell')]
phase=0;seat=0;last=time.monotonic();started=last

def tick(dt):
 global phase,seat,last
 try:
  if time.monotonic()-last<.4:return
  last=time.monotonic()
  if last-started>150:raise RuntimeError('Travel validation timed out')
  w,pc,pawn,game=state()
  if not all([w,pc,pawn,game]):return
  if phase==0:
   x,y,name=seats[seat];pawn.set_actor_location(u.Vector(x,y,165),False,True)
   check('Seat prompt '+name,name in pc.get_interaction_prompt());pc.interact();phase=1
  elif phase==1:
   check('Conversation opens '+seats[seat][2],pc.is_screen_open());pc.close_screen();seat+=1
   phase=0 if seat<len(seats) else 2
  elif phase==2:
   pawn.set_actor_location(u.Vector(-770,260,165),False,True);check('Return door prompt','return to the Oval Office' in pc.get_interaction_prompt());pc.interact();phase=3
  elif phase==3:
   if 'CabinetRoom' in w.get_name():return
   check('Arrived in Oval Office','OvalOffice' in w.get_name());check('Same presidency instance',u.GameplayStatics.get_game_instance(w)==initial_instance);check('Term values survive return',fingerprint(game)==initial_state)
   loc=pawn.get_actor_location();check('Safe Oval Office arrival',abs(loc.x-70)<2 and abs(loc.y-310)<2 and pawn.can_stand(u.Vector2D(loc.x,loc.y)))
   pawn.set_actor_location(u.Vector(30,370,165),False,True);check('Cabinet door prompt','visit the Cabinet Room' in pc.get_interaction_prompt());pc.interact();phase=4
  elif phase==4:
   if 'CabinetRoom' not in w.get_name():return
   check('Arrived back in Cabinet Room',True);check('Term values survive full round trip',fingerprint(game)==initial_state);check('Saved term byte-for-byte unchanged',save.read_bytes()==initial)
   loc=pawn.get_actor_location();check('Safe Cabinet Room arrival',abs(loc.x+575)<2 and abs(loc.y-280)<2 and pawn.can_stand(u.Vector2D(loc.x,loc.y)))
   pawn.set_actor_location(u.Vector(-575,280,165),False,True);pc.set_control_rotation(u.Rotator(pitch=-2,yaw=-18,roll=0))
   (P/'Saved/CabinetValidation.json').write_text(json.dumps({'status':'passed','checks':checks,'save_sha256':hashlib.sha256(initial).hexdigest()},indent=2));u.log('CABINET VALIDATION PASSED: '+str(len(checks))+' checks');u.unregister_slate_post_tick_callback(handle)
 except Exception:
  (P/'Saved/CabinetValidation.json').write_text(json.dumps({'status':'failed','checks':checks,'error':traceback.format_exc()},indent=2));u.unregister_slate_post_tick_callback(handle);raise
handle=u.register_slate_post_tick_callback(tick)
