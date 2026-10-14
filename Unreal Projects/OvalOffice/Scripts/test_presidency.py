"""Play-in-Editor acceptance test. Backs up/restores the save and stops PIE after testing.
Starts on any native presidency map; uses controller travel rather than loading editor assets.
"""
import unreal as u,json,time,traceback,hashlib
from pathlib import Path
P=Path(u.Paths.project_dir());editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
save=P/'Saved/FourYears/Term.json';original=save.read_bytes();backup=save.with_name('Term.before-presidency-smoke.json');backup.write_bytes(original)
archive=save.with_name('PreviousTerm.json');old_archive=archive.read_bytes() if archive.exists() else None
checks=[];phase=0;last=0;started=time.monotonic();initial=None

def state():
 w=editor.get_game_world()
 if not w:return None,None,None,None
 pc=u.GameplayStatics.get_player_controller(w,0);pawn=u.GameplayStatics.get_player_pawn(w,0);gi=u.GameplayStatics.get_game_instance(w)
 game=next((o for o in u.ObjectIterator(u.FourYearsSubsystem) if o.get_outer()==gi),None)
 return w,pc,pawn,game

def check(name,ok):
 checks.append({'check':name,'passed':bool(ok)})
 if not ok:raise AssertionError(name)

def line(pawn,a,b,label):
 for i in range(25):
  t=i/24;x=a[0]*(1-t)+b[0]*t;y=a[1]*(1-t)+b[1]*t
  check(label+' '+str(i),pawn.can_stand(u.Vector2D(x,y)))

def finish(status,error=''):
 u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
 save.write_bytes(original)
 if old_archive is not None:archive.write_bytes(old_archive)
 elif archive.exists():archive.unlink()
 report={'status':status,'checks':checks,'error':error,'save_restored':save.read_bytes()==original,'save_sha256':hashlib.sha256(original).hexdigest()}
 (P/'Saved/PresidencyValidation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 u.unregister_slate_post_tick_callback(presidency_test_handle)
 u.log('PRESIDENCY VALIDATION '+status+': '+str(len(checks))+' checks')

def tick(dt):
 global phase,last,initial
 try:
  if time.monotonic()-last<1.3:return
  last=time.monotonic()
  if last-started>240:raise RuntimeError('Presidency travel timed out')
  w,pc,pawn,game=state()
  if not all([w,pc,pawn,game]):return
  view=json.loads(game.get_presidency_json())
  if phase==0:
   initial=(game.get_briefing().quarter,game.get_status().capital);check('Native walker',isinstance(pawn,u.FourYearsWalker));pc.navigate_presidency('travel:residence');phase=1
  elif phase==1:
   check('Residence entered',view['place']=='residence');check('Residence eye height',abs(pawn.get_actor_location().z-165)<1)
   route=[(8235,0),(8350,0),(8350,-110),(8630,-110),(8630,0),(8770,0),(8770,140),(8670,140),(8330,140),(8235,0)]
   for a,b in zip(route,route[1:]):line(pawn,a,b,'Residence route')
   check('Residence coffee table blocks entry',not pawn.can_stand(u.Vector2D(8500,66)))
   check('Residence wall blocks entry',not pawn.can_stand(u.Vector2D(8900,0)))
   pc.navigate_presidency('travel:limo');phase=2
  elif phase==2:
   check('Limo entered',view['place']=='limo');check('Limo camera height',abs(pawn.get_actor_location().z-145)<1)
   line(pawn,(4780,0),(5180,0),'Cabin aisle');check('Cabin side wall blocks',not pawn.can_stand(u.Vector2D(5000,90)))
   pc.navigate_presidency('travel:cabinet');phase=3
  elif phase==3:
   if 'CabinetRoom' not in w.get_name():return
   check('Cabinet map uses presidency mode',isinstance(pc,u.FourYearsPlayerController));check('Cabinet place registered',view['place']=='cabinet')
   line(pawn,(-570,280),(570,280),'Cabinet perimeter');check('Cabinet table blocked',not pawn.can_stand(u.Vector2D(0,0)))
   staff=u.GameplayStatics.get_all_actors_of_class(w,u.FourYearsStaff);check('Cabinet populated',sum(a.place=='cabinet' for a in staff)>=4)
   pc.navigate_presidency('relations');phase=4
  elif phase==4:
   check('Government screen opens',pc.is_screen_open());pc.navigate_presidency('travel:aircraft');phase=5
  elif phase==5:
   if 'AirForceOne' not in w.get_name():return
   check('Aircraft native walker',isinstance(pawn,u.FourYearsWalker));check('Aircraft place registered',view['place']=='aircraft')
   line(pawn,(70,276),(1200,276),'Aircraft corridor')
   for xy in [(365,95),(650,150)]:check('Aircraft room reachable point '+str(xy),pawn.can_stand(u.Vector2D(*xy)))
   check('Aircraft outside blocked',not pawn.can_stand(u.Vector2D(500,360)))
   pc.navigate_presidency('travel:oval');phase=6
  elif phase==6:
   if 'OvalOffice' not in w.get_name():return
   check('Oval round trip',view['place']=='oval');check('Progress survives all travel',(game.get_briefing().quarter,game.get_status().capital)==initial)
   staff=u.GameplayStatics.get_all_actors_of_class(w,u.FourYearsStaff);check('Oval populated',sum(a.place=='oval' for a in staff)>=3)
   npc=next(a for a in staff if a.person_name=='Maya Chen');pos=npc.get_actor_location();pawn.set_actor_location(u.Vector(pos.x-75,pos.y,165),False,True);pc.close_screen()
   check('Named NPC prompt','Maya Chen' in pc.get_interaction_prompt());pc.interact();phase=7
  elif phase==7:
   check('NPC conversation opens',pc.is_screen_open());pc.navigate_presidency('story');phase=8
  elif phase==8:
   check('Chapter journal opens',pc.is_screen_open());check('Three responses presented',len(view['chapter']['choices'])==3)
   # Exercise the subsystem bridge on a disposable new term, then restore disk and stop PIE.
   game.start_new_term();game.complete_inauguration();check('Native party choice',game.choose_electoral_party('democrat').ok);check('Native mandate',game.presidency_action('mandate','homes').ok)
   pc.navigate_presidency('travel:residence');phase=9
  elif phase==9:
   check('Native story response',game.presidency_action('story','0').ok);check('Native rest action',game.presidency_action('rest','').ok)
   check('Native chapter duplicate refused',not game.presidency_action('story','0').ok)
   check('Native desk decision',game.choose_response(0));r=game.advance_quarter();check('Native quarter advances',game.get_briefing().quarter==1)
   check('Next chapter delivered',json.loads(game.get_presidency_json())['chapter']['title']=='The ride to Capitol Hill')
   finish('passed')
 except Exception:finish('failed',traceback.format_exc())
presidency_test_handle=u.register_slate_post_tick_callback(tick)
