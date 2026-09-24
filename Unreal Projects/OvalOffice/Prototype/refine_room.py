from pathlib import Path
p=Path(__file__).parent/'office-game.template.html'
s=p.read_text(encoding='utf-8')
(p.parent/'office-game.before-polish.html').write_text(s,encoding='utf-8')
s=s.replace("import {OrbitControls}","import {mergeGeometries} from 'https://esm.sh/three@0.170.0/examples/jsm/utils/BufferGeometryUtils.js';\nimport {RoomEnvironment} from 'https://esm.sh/three@0.170.0/examples/jsm/environments/RoomEnvironment.js';\nimport {OrbitControls}")
s=s.replace("lastX=0,yaw=0", "lastX=0,lastY=0,yaw=0,pitch=-.12")
start=s.index("const canvas=$('scene');")
end=s.index('function shellVisibility()',start)
s=s[:start]+'''const canvas=$('scene');
renderer=new THREE.WebGLRenderer({canvas,antialias:true,powerPreference:'high-performance'});
renderer.setPixelRatio(Math.min(devicePixelRatio,1.75));renderer.outputColorSpace=THREE.SRGBColorSpace;
renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1.06;
renderer.shadowMap.enabled=true;renderer.shadowMap.type=THREE.PCFSoftShadowMap;renderer.shadowMap.autoUpdate=false;
scene=new THREE.Scene();scene.background=new THREE.Color(0xb9c9ca);
const pmrem=new THREE.PMREMGenerator(renderer),environment=new RoomEnvironment();
scene.environment=pmrem.fromScene(environment,.05).texture;scene.environmentIntensity=.24;environment.dispose();pmrem.dispose();
camera=new THREE.PerspectiveCamera(58,1,.05,100);camera.position.set(0,1.65,3.85);camera.rotation.order='YXZ';camera.rotation.set(pitch,yaw,0);
scene.add(new THREE.HemisphereLight(0xfff3db,0xa6957c,1.8));
let sun=new THREE.DirectionalLight(0xffedce,3.0);sun.position.set(-3.5,6,-7);sun.target.position.set(1,0,2);scene.add(sun,sun.target);
sun.castShadow=true;sun.shadow.mapSize.set(2048,2048);Object.assign(sun.shadow.camera,{left:-7,right:7,top:8,bottom:-6,near:.5,far:25});sun.shadow.bias=-.00025;sun.shadow.normalBias=.025;sun.shadow.radius=3;
let sky=new THREE.DirectionalLight(0xe8efff,.8);sky.position.set(0,4,3);scene.add(sky);
orbit=new OrbitControls(camera,canvas);orbit.enabled=false;orbit.enableDamping=false;orbit.target.set(0,1.6,0);orbit.minDistance=3;orbit.maxDistance=24;
''' + s[end:]
s=s.replace("camera.lookAt(0,2.55,5.05)","camera.lookAt(0,2.02,5.05)")
s=s.replace("yaw=0;camera.rotation.set(0,yaw,0)","yaw=0;pitch=-.12;camera.rotation.set(pitch,yaw,0)")
s=s.replace("shellVisibility()}\n$('walk')", "shellVisibility();renderer.shadowMap.needsUpdate=true}\n$('walk')")
s=s.replace("lastX=e.clientX;canvas.setPointerCapture", "lastX=e.clientX;lastY=e.clientY;canvas.setPointerCapture")
s=s.replace("lastX=e.clientX});canvas.addEventListener", "pitch=clamp(pitch-(e.clientY-lastY)/canvas.clientHeight*1.8,-.7,.65);lastY=e.clientY;lastX=e.clientX});canvas.addEventListener")
s=s.replace("camera.rotation.y=yaw;$('prompt')", "camera.rotation.set(pitch,yaw,0);$('prompt')")
s=s.replace("if(o.name.startsWith('ROOF__'))o.material=new THREE.MeshBasicMaterial({color:0xece7dc,side:THREE.DoubleSide,toneMapped:false})", "if(o.name.startsWith('ROOF__'))o.material=new THREE.MeshStandardMaterial({color:0xebe4d6,roughness:1,side:THREE.DoubleSide})")
s=s.replace('scene.add(model);shellVisibility();addRoomDetails();', 'scene.add(model);shellVisibility();addRoomDetails();polishRoom();renderer.shadowMap.needsUpdate=true;')
s=s.replace('</script>\n</body>', '__ROOM_POLISH__\n</script>\n</body>')
# Use the available viewport for the room, including narrow desktop panels.
s=s.replace('</style>', '''
.shell{height:100dvh;min-height:560px;grid-template-rows:auto minmax(0,1fr)}
.stage{min-height:0}.vignette{background:radial-gradient(ellipse at center,transparent 48%,#21190f22 100%)}
.controls{max-width:calc(100% - 44px)}.chip{background:#152632dc;backdrop-filter:blur(10px);border-color:#c8b28277;padding:10px 13px}
.hint{background:#192b35cc;color:#e9e2d4}.mast{padding:13px 22px}
@media(max-width:760px){.shell{height:100dvh;min-height:540px;grid-template-rows:auto minmax(0,1fr)}.stage{min-height:0}.controls{max-width:calc(100% - 16px);gap:5px}.chip{font-size:10px;padding:8px 10px}.hint{top:auto;bottom:10px;max-width:calc(100% - 16px);font-size:10px}.mast{padding:9px 14px}.brand{font-size:20px}.panel{max-height:66%}}
</style>''')
p.write_text(s,encoding='utf-8')
print('Updated room materials, lighting, camera and viewport layout')
