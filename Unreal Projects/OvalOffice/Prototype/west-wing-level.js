// Authored in Blender. Coordinates are metres; Blender (x,y,z) -> glTF (x,z,-y).
const westWingEncoded='__WEST_WING_BASE64__';
const westWingNav=__WEST_WING_NAV__;
let westWingScene,westWingModel,westWingPromise,westWingRoom='';
function westWingCanWalk(x,z){
 const y=-z,r=westWingNav.radius;
 // Test a capsule footprint against the union of floors, so joined doorways stay open.
 for(let i=0;i<9;i++){
  const a=i*Math.PI/4,px=x+(i===8?0:Math.cos(a)*r),py=y+(i===8?0:Math.sin(a)*r);
  if(!westWingNav.floors.some(([x1,y1,x2,y2])=>px>=x1&&px<=x2&&py>=y1&&py<=y2))return false;
 }
 for(const {rect:[x1,y1,x2,y2]} of westWingNav.solids){
  const dx=Math.max(x1-x,0,x-x2),dy=Math.max(y1-y,0,y-y2);
  if(dx*dx+dy*dy<r*r)return false;
 }
 return true;
}
function westWingZone(){
 const x=camera.position.x,y=-camera.position.z;
 return westWingNav.zones.slice().reverse().find(z=>{const [a,b,c,d]=z.rect;return x>=a&&x<=c&&y>=b&&y<=d});
}
function westWingAction(){
 if(Math.hypot(camera.position.x-westWingNav.ovalPortal[0],-camera.position.z-westWingNav.ovalPortal[1])<1.15)return {destination:'oval',label:'Press E to enter the Oval Office'};
 const zone=westWingZone();
 if(zone?.id!==westWingRoom){westWingRoom=zone?.id||'hall';$('location-name').textContent=zone?.name||'West Wing galleries'}
 if(zone?.page)return {page:zone.page,label:'Press E to '+({team:'review your Cabinet',congress:'review the legislative agenda',journal:'read the presidential record',moments:'open press & crisis briefings',appointments:'review your appointments'}[zone.page]||'open your dashboard')};
 return null;
}
async function loadWestWing(){
 if(westWingScene)return;if(westWingPromise)return westWingPromise;
 westWingPromise=(async()=>{
  const bytes=Uint8Array.from(atob(westWingEncoded),c=>c.charCodeAt(0));
  const glb=await new Response(new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip'))).arrayBuffer();
  const asset=await new GLTFLoader().parseAsync(glb,'');asset.scene.updateMatrixWorld(true);
  const model=new THREE.Group();model.name='Blender West Wing';const batches=new Map();
  asset.scene.traverse(o=>{
   if(!o.isMesh)return;
   const mat=o.material;if(Array.isArray(mat))throw new Error('Unexpected West Wing multi-material primitive');
   const n=o.name.replaceAll('_',' '),cutaway=/^(ROOF|UPPER)/.test(n);
   mat.side=THREE.DoubleSide;mat.envMapIntensity=.45;
   if(mat.name.includes('glazing')){mat.transparent=true;mat.opacity=.10;mat.depthWrite=false}
   if(mat.name.includes('luminous')){mat.emissive.set(0xffe3b0);mat.emissiveIntensity=.7}
   if(mat.map)mat.map.anisotropy=Math.min(8,renderer.capabilities.getMaxAnisotropy());
   const key=mat.uuid+'|'+cutaway;if(!batches.has(key))batches.set(key,{mat,cutaway,geometries:[]});
   let geo=o.geometry.clone();if(geo.index)geo=geo.toNonIndexed();geo.applyMatrix4(o.matrixWorld);
   for(const k of Object.keys(geo.attributes))if(!['position','normal','uv'].includes(k))geo.deleteAttribute(k);
   if(!geo.attributes.uv)geo.setAttribute('uv',new THREE.Float32BufferAttribute(new Float32Array(geo.attributes.position.count*2),2));
   if(!geo.attributes.normal)geo.computeVertexNormals();batches.get(key).geometries.push(geo);
  });
  for(const {mat,cutaway,geometries} of batches.values()){
   const geo=mergeGeometries(geometries,false);if(!geo)throw new Error('West Wing geometry batching failed');
   const mesh=new THREE.Mesh(geo,mat);mesh.userData.cutaway=cutaway;mesh.castShadow=!mat.transparent;mesh.receiveShadow=true;model.add(mesh);geometries.forEach(g=>g.dispose());
  }
  asset.scene.traverse(o=>{if(o.isMesh)o.geometry.dispose()});
  const room=new THREE.Scene();room.background=new THREE.Color(0xc6dce4);room.environment=ovalScene.environment;room.environmentIntensity=.38;room.add(model);
  room.add(new THREE.HemisphereLight(0xfff3df,0x8b8976,1.65));
  const sun=new THREE.DirectionalLight(0xffe5ba,2.2);sun.position.set(22,32,6);sun.target.position.set(3,0,-10);sun.castShadow=true;sun.shadow.mapSize.set(2048,2048);Object.assign(sun.shadow.camera,{left:-37,right:37,top:37,bottom:-37,near:.1,far:90});sun.shadow.normalBias=.04;room.add(sun,sun.target);
  // Non-shadowed pools give the authored ceiling fixtures a soft, economical bounce.
  const pools=[[3,3],[3,8],[3,14],[3,19],[-8.5,4],[-8.5,11],[-8.5,18],[-3,3],[-3,14],[-3,22.5],[-2.75,8.5],[-2.75,-1.9],[8.75,10],[8.75,16],[9.2,24],[9.2,30]];
  for(const [x,y] of pools){const lamp=new THREE.PointLight(0xffdeb2,15,8,2);lamp.position.set(x,2.95,-y);room.add(lamp)}
  westWingScene=room;westWingModel=model;
 })();
 try{await westWingPromise}catch(error){westWingPromise=null;throw error}
}
function setWestWingPose(id='hall'){
 if(currentLocation!=='westwing')return;
 const z=westWingNav.zones.find(z=>z.id===id)||westWingNav.zones[0];
 mode='walk';orbit.enabled=false;placeCamera([z.spawn[0],1.65,-z.spawn[1]],[z.target[0],1.55,-z.target[1]]);
 $('walk').classList.add('active');$('overview').classList.remove('active');$('fireplace').classList.remove('active');
 $('hint').textContent='W/A/S/D or arrows to walk · drag to look · E at the Oval doorway or in meeting rooms · Wing map for directions';
 if($('wing-room'))$('wing-room').value=z.id;
 westWingRoom='';westWingAction();keys.clear();tapKeys.clear();shellVisibility();renderer.shadowMap.needsUpdate=true;
}
function renderWestWingMap(){
 $('panel').dataset.page='westwing-map';
 const colors={hall:'#ab9980',cabinet:'#719d96',roosevelt:'#a28063',study:'#848f73',lobby:'#aeb2a0',press:'#658aac',colonnade:'#c7b58d',garden:'#668d59'};
 const project=([x,y])=>[25+(x+18)*8,355-(y+11)*7];
 $('panel').innerHTML=header('EXPLORE / WEST WING','Beyond the Oval Office','Walk through connected galleries and meeting rooms, or select a destination below. Returning to your desk preserves the term.')+`<svg class="wing-floorplan" viewBox="0 0 470 365" role="img" aria-label="West Wing floor plan. The Cabinet and Roosevelt rooms connect to the main galleries; reception leads to the press room; the Cabinet garden door leads to the colonnade and garden."><rect x="0" y="0" width="470" height="365" rx="10" fill="#152b3a"/>${westWingNav.floors.map(([a,b,c,d])=>{const [x,y]=project([a,d]);return `<rect x="${x}" y="${y}" width="${(c-a)*8}" height="${(d-b)*7}" fill="#3e5665" stroke="#8aa3af" stroke-width=".6"/>`}).join('')}${westWingNav.zones.filter(z=>z.id!=='hall').map(z=>{const [a,b,c,d]=z.rect,[x,y]=project([a,d]);return `<g role="button" tabindex="0" data-wing-place="${z.id}" aria-label="Visit ${z.name}"><rect x="${x+2}" y="${y+2}" width="${(c-a)*8-4}" height="${(d-b)*7-4}" rx="3" fill="${colors[z.id]}"/><text x="${x+(c-a)*4}" y="${y+(d-b)*3.5}" text-anchor="middle" fill="#10232d" font-size="9" font-family="system-ui">${z.id==='roosevelt'?'Roosevelt':z.id==='press'?'Press':z.id==='colonnade'?'Colonnade':z.id==='garden'?'Rose Garden':z.id==='cabinet'?'Cabinet':z.id==='study'?'Study':'Reception'}</text></g>`}).join('')}<ellipse cx="${project([8.4,0])[0]}" cy="${project([8.4,0])[1]}" rx="36" ry="38" fill="#c3a66e"/><text x="${project([8.4,0])[0]}" y="${project([8.4,0])[1]}" text-anchor="middle" fill="#152737" font-size="9">Oval Office</text></svg><p class="note">Public-tour-inspired layout, adapted for play. The staff offices and upper storey are closed. The original Oval Office loads through its doorway.</p><div class="wing-destinations">${westWingNav.zones.map(z=>`<button class="secondary" data-wing-place="${z.id}">${z.name} ↗</button>`).join('')}</div><div class="actions"><button class="primary" id="wing-back-oval">Return to the Oval Office</button><button class="secondary" id="wing-resume">Continue walking</button></div>`;
 $('panel').querySelectorAll('[data-wing-place]').forEach(b=>{const action=()=>{setWestWingPose(b.dataset.wingPlace);$('panel').hidden=true;$('briefing').textContent='Open strategy'};b.onclick=action;b.onkeydown=e=>{if((e.key==='Enter'||e.key===' ')&&b.tagName.toLowerCase()==='g'){e.preventDefault();action()}}});
 $('wing-back-oval').onclick=()=>changeLocation('oval');$('wing-resume').onclick=()=>{$('panel').hidden=true;$('briefing').textContent='Open strategy'};
}
if($('westwing'))$('westwing').onclick=()=>changeLocation(currentLocation==='westwing'?'oval':'westwing');
if($('wing-map'))$('wing-map').onclick=()=>openPage('westwing-map');
if($('wing-room')){$('wing-room').innerHTML=westWingNav.zones.map(z=>`<option value="${z.id}">${z.name}</option>`).join('');$('wing-room').onchange=e=>setWestWingPose(e.target.value)}
