import bpy,json,sys
from pathlib import Path
P=Path(__file__).parent;s=bpy.context.scene
mode=sys.argv[-1]
if mode=='export':
 bpy.ops.object.select_all(action='DESELECT')
 for o in s.objects:
  if o.type in {'MESH','CURVE','FONT','EMPTY'} and not o.name.startswith('EXTERIOR'):
   o.select_set(True)
 bpy.ops.export_scene.gltf(filepath=str(P/'Air_Force_One.glb'),export_format='GLB',use_selection=True,export_apply=True,export_cameras=False,export_lights=False,export_extras=True)
 report={'project':'Air Force One compact presidential suite','blender_file':'Air_Force_One.blend','portable_model':'Air_Force_One.glb','units':'metres','rooms':['Presidential office','Connecting hallway','Conference room'],'dimensions_metres':[5.2,11.8,2.6],'door_clear_width_metres':1.0,'ceiling_metres':2.4,'objects':len(s.objects),'materials':len(bpy.data.materials),'cameras':[o.name for o in s.objects if o.type=='CAMERA'],'game_markers':{o.name:list(o.location) for o in s.objects if o.type=='EMPTY'},'layout_note':'Interpretive game layout based on public references; not a surveyed replica.','integration_status':'Asset export only; collision and level transitions still need game-engine integration.'}
 if s.get('Walkability revision'):
  walk=json.loads((P/'walkability_report.json').read_text())
  report.update(dimensions_metres=[7.2,13.1,2.6],door_clear_width_metres=walk['door_clear_width_m'],hall_clear_width_metres=walk['hall_clear_width_m'],walkability_report='walkability_report.json',integration_status='Asset export with geometry clearance tests passed. Engine collision, navigation and level transitions still need integration.')
 (P/'level_manifest.json').write_text(json.dumps(report,indent=2));print('EXPORT_READY',flush=True)
else:
 cameras={'office':('01 | Presidential office','Air_Force_One_Office.png'),'conference':('02 | Conference room','Air_Force_One_Conference.png'),'hallway':('03 | Connecting hallway','Air_Force_One_Hallway.png'),'cutaway':('04 | Three-room cutaway','Air_Force_One_Cutaway.png')}
 name,file=cameras[mode];s.camera=bpy.data.objects[name]
 if mode=='cutaway':
  for o in s.objects:
   if o.name.startswith(('CEILING','EXTERIOR','Headliner','Hall outboard','Hall end','Passenger service inset','Cabin air nozzle','Recessed ceiling luminous strip','Hall overhead diffuser','Cabin ceiling service reveal')):o.hide_render=True
  s.world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.8
  d=bpy.data.lights.new('Cutaway studio softbox','AREA');d.energy=1800;d.shape='DISK';d.size=10;o=bpy.data.objects.new('Cutaway studio softbox',d);s.collection.objects.link(o);o.location=(0,6,12)
 s.render.resolution_x=1500;s.render.resolution_y=1000;s.cycles.samples=64
 s.render.filepath=str(P/'renders'/file);bpy.ops.render.render(write_still=True)
