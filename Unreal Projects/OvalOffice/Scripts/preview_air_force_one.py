"""Capture native game views and leave the player facing the updated passage."""
import unreal as u,time
from pathlib import Path
P=Path(u.Paths.project_dir())/'Saved'/'AirForceOne';started=time.monotonic();stage=0;since=0
views=[('AirForceOne_Unreal_Office',(365,95,94),-130),('AirForceOne_Unreal_Hallway',(70,276,94),0),('AirForceOne_Unreal_Conference',(650,150,94),-27)]
def callback(dt):
 global stage,since
 world=u.find_object(None,'/Game/AirForceOne/Maps/AirForceOne.AirForceOne')
 if not world:return
 pawn=u.GameplayStatics.get_player_pawn(world,0)
 if not pawn:return
 now=time.monotonic()
 if stage==0:
  if now-started<8:return
  since=now;stage=1
 if stage in [1,3,5]:
  label,loc,yaw=views[(stage-1)//2];pawn.set_actor_location(u.Vector(*loc),False,True);pawn.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately();u.GameplayStatics.get_player_controller(world,0).set_control_rotation(u.Rotator(pitch=-3,yaw=yaw,roll=0));since=now;stage+=1
 elif stage in [2,4,6] and now-since>3:
  label,loc,yaw=views[(stage-2)//2];u.SystemLibrary.execute_console_command(world,'Shot filename="'+(P/(label+'.png')).as_posix()+'" nosuffix');stage+=1;since=now
 elif stage==7 and now-since>2:
  pawn.set_actor_location(u.Vector(70,276,94),False,True);u.GameplayStatics.get_player_controller(world,0).set_control_rotation(u.Rotator(pitch=0,yaw=0,roll=0));u.unregister_slate_post_tick_callback(handle);(P/'preview_complete.txt').write_text('Native Unreal screenshots captured. Player facing the updated passage.')
handle=u.register_slate_post_tick_callback(callback)
