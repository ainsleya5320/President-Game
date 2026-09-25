"""Observe real keyboard input and resulting motion; never drive the character."""
import unreal as u,time,json
from pathlib import Path
P=Path(u.Paths.project_dir());out=P/'Saved'/'AirForceOne'/'keyboard_audit.json';out.parent.mkdir(exist_ok=True)
keys={}
for name in 'WASD':
 k=u.Key();assert k.import_text(name);keys[name]=k
report={'method':'Observed physical keyboard input and resulting character displacement; no movement injected','keys':{},'ready':False};start=time.monotonic();last=None;previous=[];next_flush=0
def xyz(p):return [round(p.x,3),round(p.y,3),round(p.z,3)]
def tick(dt):
 global last,previous,next_flush
 world=u.find_object(None,'/Game/AirForceOne/Maps/AirForceOne.AirForceOne')
 if not world:return
 pc=u.GameplayStatics.get_player_controller(world,0);pawn=u.GameplayStatics.get_player_pawn(world,0)
 if not pc or not pawn:return
 now=time.monotonic();p=pawn.get_actor_location();held=[name for name,key in keys.items() if pc.is_input_key_down(key)];pressed=[name for name,key in keys.items() if pc.was_input_key_just_pressed(key)]
 report['ready']=True;report['position']=xyz(p)
 for name in set(held+pressed):
  item=report['keys'].setdefault(name,{'input_frames':0,'distance_cm':0,'first_position':xyz(p)})
  item['input_frames']+=1
 if last:
  distance=((p.x-last.x)**2+(p.y-last.y)**2)**.5
  for name in previous:
   item=report['keys'][name];item['distance_cm']=round(item['distance_cm']+distance,3);item['last_position']=xyz(p)
 previous=held;last=p
 if now>=next_flush:out.write_text(json.dumps(report,indent=2));next_flush=now+.4
 if now-start>300:u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
