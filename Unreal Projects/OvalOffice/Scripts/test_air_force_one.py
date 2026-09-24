"""Opt-in runtime acceptance test, invoked only by the review launcher."""
import unreal as u,json,time,traceback,math
from pathlib import Path
P=Path(u.Paths.project_dir());OUT=P/'Saved'/'AirForceOne';OUT.mkdir(exist_ok=True)
source=json.loads((P.parent.parent/'air-force-one'/'walkability_report.json').read_text())
routes=list(source['paths'].items());report={'status':'running','routes':[]};world=None;pawn=None;stage='wait';started=time.monotonic();route_index=0;point_index=0;stage_time=0;last_progress=0
def flush():(OUT/'runtime_test.json').write_text(json.dumps(report,indent=2))
def position(v):return [round(v.x,2),round(v.y,2),round(v.z,2)]
def shot(name):u.SystemLibrary.execute_console_command(world,'Shot filename="'+(OUT/(name+'.png')).as_posix()+'" nosuffix')
def reset_hall():
 pawn.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately();pawn.set_actor_location(u.Vector(360,275,94),False,True)
def callback(dt):
 global world,pawn,stage,stage_time,route_index,point_index,last_progress
 try:
  now=time.monotonic()
  if world is None:
   world=u.find_object(None,'/Game/AirForceOne/Maps/AirForceOne.AirForceOne')
   if world is None:
    if now-started>90:raise RuntimeError('Game world did not load')
    return
  if pawn is None:
   pawn=u.GameplayStatics.get_player_pawn(world,0)
   if pawn is None:return
   report['pawn_class']=pawn.get_class().get_name();cap=pawn.get_component_by_class(u.CapsuleComponent)
   report['trajectory']=[]
   if cap is None:raise RuntimeError('No walking capsule on pawn')
   report['capsule_radius_cm']=cap.get_scaled_capsule_radius();report['capsule_half_height_cm']=cap.get_scaled_capsule_half_height();stage_time=now;stage='settle';flush()
  if stage=='settle':
   if now-stage_time<12:return
   p=pawn.get_actor_location();report['initial_position']=position(p);report['floor_support_pass']=89<p.z<100
   report['camera_position']=position(u.GameplayStatics.get_player_camera_manager(world,0).get_camera_location());shot('Unreal_Office');stage='afteroffice';stage_time=now
  elif stage=='afteroffice':
   if now-stage_time<2:return
   reset_hall();stage='routes';stage_time=now;last_progress=now
  elif stage=='routes':
   name,path=routes[route_index];p=pawn.get_actor_location();tx,ty=path[point_index][1]*100,path[point_index][0]*100;dx,dy=tx-p.x,ty-p.y;dist=math.hypot(dx,dy)
   if len(report['trajectory'])<80:report['trajectory'].append({'dt':dt,'p':position(p),'target':[tx,ty],'velocity':position(pawn.get_velocity()),'input':position(pawn.get_pending_movement_input_vector())})
   if dist<6:
    point_index+=1;last_progress=now
    if point_index==len(path):
     report['routes'].append({'name':name,'passed':True,'seconds':round(now-stage_time,2),'position':position(p)});flush();route_index+=1;point_index=0;reset_hall();stage_time=now;last_progress=now
     if route_index==len(routes):stage='wall';stage_time=now
    return
   if now-last_progress>12:raise RuntimeError('Movement blocked on '+name+' at '+str(position(p))+' target '+str([tx,ty]))
   if p.z<70:raise RuntimeError('Player fell through floor')
   movement=pawn.get_component_by_class(u.CharacterMovementComponent);movement.set_editor_property('max_walk_speed',100)
   pawn.add_movement_input(u.Vector(dx/dist,dy/dist,0),min(1,dist/max(3,100*dt)),True)
  elif stage=='wall':
   pawn.add_movement_input(u.Vector(0,1,0),1,True)
   if now-stage_time<3:return
   p=pawn.get_actor_location();report['wall_stop_position']=position(p);report['wall_collision_pass']=300<p.y<320
   pawn.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately();pawn.set_actor_location(u.Vector(70,276,94),False,True);u.GameplayStatics.get_player_controller(world,0).set_control_rotation(u.Rotator(pitch=0,yaw=0,roll=0));stage='hallshot';stage_time=now
  elif stage=='hallshot':
   if now-stage_time<3:return
   shot('Unreal_Hallway');pawn.set_actor_location(u.Vector(650,150,94),False,True);u.GameplayStatics.get_player_controller(world,0).set_control_rotation(u.Rotator(pitch=-4,yaw=-27,roll=0));stage='conferenceshot';stage_time=now
  elif stage=='conferenceshot':
   if now-stage_time<3:return
   shot('Unreal_Conference');pawn.set_actor_location(u.Vector(365,95,94),False,True);u.GameplayStatics.get_player_controller(world,0).set_control_rotation(u.Rotator(pitch=0,yaw=-130,roll=0));pawn.get_component_by_class(u.CharacterMovementComponent).set_editor_property('max_walk_speed',180)
   report['status']='passed' if report['floor_support_pass'] and report['wall_collision_pass'] and len(report['routes'])==7 else 'failed';flush();u.unregister_slate_post_tick_callback(handle)
 except Exception:
  report['status']='failed';report['error']=traceback.format_exc();flush();u.unregister_slate_post_tick_callback(handle)
flush();handle=u.register_slate_post_tick_callback(callback)
