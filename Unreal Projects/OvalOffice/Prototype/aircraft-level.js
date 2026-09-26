// The same Blender cabin used by the Unreal level, with browser-specific lighting.
// Blender coordinates (x,y,z) become Three.js (x,z,-y) in the glTF export.
const aircraftEncoded='__AIRCRAFT_BASE64__';
const aircraftFootprints=__AIRCRAFT_COLLISION__;
let ovalScene,ovalModel,aircraftScene,aircraftModel,aircraftPromise;
const roomPoses={};
const aircraftSolids=aircraftFootprints.map(p=>({p,minX:Math.min(...p.map(v=>v[0]))-.34,maxX:Math.max(...p.map(v=>v[0]))+.34,minY:Math.min(...p.map(v=>v[1]))-.34,maxY:Math.max(...p.map(v=>v[1]))+.34}));

function aircraftCanWalk(x,z){
 const y=-z,radius=.34;
 if(x < -3.4 || x > 3.5 || y < .10 || y > 12.83)return false;
 for(const {p,minX,maxX,minY,maxY} of aircraftSolids){
  if(x<minX||x>maxX||y<minY||y>maxY)continue;
  let inside=true;
  for(let i=0;i<p.length;i++){
   const a=p[i],b=p[(i+1)%p.length],dx=b[0]-a[0],dy=b[1]-a[1];
   if((x-a[0])*dy-(y-a[1])*dx>1e-8)inside=false;
   const t=clamp(((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy),0,1);
   if((x-a[0]-t*dx)**2+(y-a[1]-t*dy)**2<radius*radius)return false;
  }
  if(inside)return false;
 }
 return true;
}

async function loadAircraft(){
 if(aircraftScene)return;
 if(aircraftPromise)return aircraftPromise;
 aircraftPromise=(async()=>{
  const bytes=Uint8Array.from(atob(aircraftEncoded),c=>c.charCodeAt(0));
  const glb=await new Response(new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip'))).arrayBuffer();
  const asset=await new GLTFLoader().parseAsync(glb,'');
  const cabin=new THREE.Group();cabin.name='Air Force One';
  const groups=new Map();asset.scene.updateMatrixWorld(true);
  // Merge static meshes by material and cutaway visibility for a modest draw count.
  asset.scene.traverse(o=>{
   if(!o.isMesh)return;
   const n=o.name.replaceAll('_',' '),material=o.material;
   const cutaway=/^(CEILING|Hall lower curved|Hall window belt|Sculpted oval window|Oval window inner|Cloud view through passage|Hall end bulkhead|Hall vertical panel)/i.test(n);
   if(Array.isArray(material))throw new Error('Unexpected multi-material aircraft mesh');
   material.side=THREE.DoubleSide;
   if(material.name==='Window glass'){
    material.transparent=true;material.opacity=.08;material.depthWrite=false;material.transmission=0;
   }
   if(material.name==='Aircraft window cloud panorama'){
    material.emissive.set(0xffffff);material.emissiveIntensity=.7;material.toneMapped=false;
   }
   if(material.name==='Luminous champagne cove')material.emissiveIntensity=1.3;
   const key=material.uuid+'|'+cutaway;
   if(!groups.has(key))groups.set(key,{material,cutaway,geometries:[]});
   let geometry=o.geometry.clone();if(geometry.index)geometry=geometry.toNonIndexed();
   geometry.applyMatrix4(o.matrixWorld);
   // Tangent signs are unnecessary here; normals, UVs and all geometry are retained.
   for(const attribute of Object.keys(geometry.attributes))if(!['position','normal','uv'].includes(attribute))geometry.deleteAttribute(attribute);
   if(!geometry.attributes.uv)geometry.setAttribute('uv',new THREE.Float32BufferAttribute(new Float32Array(geometry.attributes.position.count*2),2));
   if(!geometry.attributes.normal)geometry.computeVertexNormals();
   groups.get(key).geometries.push(geometry);
  });
  for(const {material,cutaway,geometries} of groups.values()){
   const geometry=mergeGeometries(geometries,false);
   if(!geometry)throw new Error('Aircraft geometry could not be combined');
   const mesh=new THREE.Mesh(geometry,material);mesh.userData.cutaway=cutaway;
   mesh.castShadow=material.name!=='Window glass'&&material.name!=='Aircraft window cloud panorama';mesh.receiveShadow=true;cabin.add(mesh);
   geometries.forEach(g=>g.dispose());
  }
  asset.scene.traverse(o=>{if(o.isMesh)o.geometry.dispose()});
  const room=new THREE.Scene();room.background=new THREE.Color(0xb9d7e6);room.environment=ovalScene.environment;room.environmentIntensity=.5;room.add(cabin);
  room.add(new THREE.HemisphereLight(0xfff7e5,0x807361,2.0));
  const daylight=new THREE.DirectionalLight(0xe8f2ff,1.8);daylight.position.set(-7,6,-5);daylight.target.position.set(0,0,-5);room.add(daylight,daylight.target);
  daylight.castShadow=true;daylight.shadow.mapSize.set(2048,2048);Object.assign(daylight.shadow.camera,{left:-9,right:9,top:9,bottom:-9,near:.1,far:30});daylight.shadow.normalBias=.03;
  // Broad ambient light plus local pools approximates the native cabin's bounced light.
  for(const y of [1.2,3.4,6.4,8.5,11]){
   const lamp=new THREE.PointLight(0xffe4bd,5.2,6,2);lamp.position.set(-.5,2.24,-y);room.add(lamp);
  }
  for(const y of [1.18,3.29,5.4,7.51,9.62,11.73]){
   const lamp=new THREE.PointLight(0xdfefff,2.5,3.2,2);lamp.position.set(3.25,1.75,-y);room.add(lamp);
  }
  // Original office windows receive a continuous sky backdrop too.
  const skyView=new THREE.Mesh(new THREE.PlaneGeometry(45,8),new THREE.MeshBasicMaterial({color:0xc4dfed,side:THREE.DoubleSide}));
  skyView.position.set(-3.9,2,-6);skyView.rotation.y=Math.PI/2;skyView.userData.cutaway=true;cabin.add(skyView);
  aircraftModel=cabin;aircraftScene=room;
 })();
 try{await aircraftPromise}catch(error){aircraftPromise=null;throw error}
}

function placeCamera(position,target){
 camera.position.set(...position);camera.lookAt(...target);yaw=camera.rotation.y;pitch=camera.rotation.x;
}

function setAircraftPose(place){
 mode='walk';orbit.enabled=false;$('walk').classList.add('active');$('overview').classList.remove('active');
 if(place==='office')placeCamera([.76,1.65,-3.6],[-.7,1.35,-1.6]);
 else if(place==='conference')placeCamera([1.5,1.65,-6.4],[-.6,1.2,-9.5]);
 else placeCamera([2.77,1.65,-1.0],[2.77,1.65,-9]);
 $('hint').textContent='W/A/S/D or arrow keys to walk · drag to look · E near a desk or the conference table';
 shellVisibility();renderer.shadowMap.needsUpdate=true;keys.clear();tapKeys.clear();
}

async function changeLocation(destination){
 if(transitioning||!ready||destination===currentLocation)return;
 if(mode==='walk')roomPoses[currentLocation]={position:camera.position.toArray(),yaw,pitch};
 transitioning=true;keys.clear();tapKeys.clear();drag=false;$('travel').disabled=true;$('loading-room').hidden=false;
 $('loading-room').textContent=destination==='aircraft'?'Preparing Air Force One…':'Returning to the Oval Office…';
 try{
  if(destination==='aircraft')await loadAircraft();
  currentLocation=destination;
  scene=destination==='aircraft'?aircraftScene:ovalScene;model=destination==='aircraft'?aircraftModel:ovalModel;
  $('travel').textContent=destination==='aircraft'?'Return to Oval Office':'Board Air Force One';
  $('walk').textContent=destination==='aircraft'?'Walk cabin':'Walk office';
  $('fireplace').textContent=destination==='aircraft'?'Presidential office':'Fireplace wall';
  $('conference').hidden=destination!=='aircraft';
  $('location-name').textContent=destination==='aircraft'?'Air Force One':'The Oval Office';
  canvas.setAttribute('aria-label','Walkable 3D '+(destination==='aircraft'?'Air Force One suite':'Oval Office'));
  $('panel').hidden=true;syncDashboardLayout();$('briefing').textContent='Open briefing';$('prompt').style.display='none';
  setMode('walk');
  const pose=roomPoses[destination];if(pose){camera.position.fromArray(pose.position);yaw=pose.yaw;pitch=pose.pitch;camera.rotation.set(pitch,yaw,0)}
  if(state){state.location=destination;save()}
  toast(destination==='aircraft'?'Welcome aboard. Your briefings and journal travel with you.':'Welcome back to the Oval Office.');
 }catch(error){console.error(error);toast('Air Force One could not load. Your term is safe; try boarding again.')}
 finally{transitioning=false;$('loading-room').hidden=true;$('travel').disabled=false}
}
