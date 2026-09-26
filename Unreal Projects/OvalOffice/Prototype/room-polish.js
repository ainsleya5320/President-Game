// Material detail and static room dressing for the playable browser scene.
function polishRoom() {
 let seed=14019;
 const random=()=>{seed=(1664525*seed+1013904223)>>>0;return seed/4294967296};
 const texture=(paint,size=512)=>{
  const c=document.createElement('canvas');c.width=c.height=size;paint(c.getContext('2d'),size);
  const t=new THREE.CanvasTexture(c);t.colorSpace=THREE.SRGBColorSpace;
  t.wrapS=t.wrapT=THREE.RepeatWrapping;t.anisotropy=Math.min(8,renderer.capabilities.getMaxAnisotropy());return t;
 };
 const wood=texture((c,s)=>{
  c.fillStyle='#97633a';c.fillRect(0,0,s,s);
  for(let i=0;i<2400;i++){const y=random()*s;c.strokeStyle=`rgba(${random()>.5?'235,187,124':'48,20,9'},${.025+random()*.1})`;c.lineWidth=.3+random()*1.7;c.beginPath();c.moveTo(0,y);for(let x=0;x<=s;x+=8)c.lineTo(x,y+Math.sin(x/65+y)*1.5+Math.sin(x/180+y*.4)*3);c.stroke()}
 });
 const linen=texture((c,s)=>{
  c.fillStyle='#e6ddc8';c.fillRect(0,0,s,s);
  for(let i=0;i<s;i+=2){c.strokeStyle=i%4?'#d4c8ae':'#f0e9db';c.globalAlpha=.34;c.beginPath();c.moveTo(i,0);c.lineTo(i,s);c.stroke();c.globalAlpha=.20;c.beginPath();c.moveTo(0,i);c.lineTo(s,i);c.stroke()}
  c.globalAlpha=1;
 });
 const stripe=texture((c,s)=>{
  const colors=['#304b60','#a78454','#d1b783','#8b5140','#e5dac0','#53696d'];
  for(let x=0;x<s;x+=16){c.fillStyle=colors[Math.floor(x/16)%6];c.fillRect(x,0,16,s);c.fillStyle='#f0e1be';c.fillRect(x+2,0,1,s)}
 });
 const marble=texture((c,s)=>{
  c.fillStyle='#e6e0d4';c.fillRect(0,0,s,s);
  for(let i=0;i<12000;i++){const v=150+Math.floor(random()*100);c.fillStyle=`rgba(${v},${v-5},${v-13},.08)`;c.fillRect(random()*s,random()*s,2+random()*9,1+random()*5)}
  for(let i=0;i<55;i++){c.strokeStyle=`rgba(118,111,104,${.025+random()*.065})`;c.lineWidth=.3+random()*2;c.beginPath();let y=random()*s;for(let x=0;x<=s;x+=7){y+=Math.sin(x/35+i)*1.6+(random()-.5)*6;c.lineTo(x,y)}c.stroke()}
 });
 // Assign physical-scale UVs to primitives whose Blender shaders used generated coordinates.
 const planarUV=(mesh,scale=1)=>{
  const g=mesh.geometry.clone(),p=g.attributes.position,n=g.attributes.normal,uv=[];
  for(let i=0;i<p.count;i++){
   const x=p.getX(i),y=p.getY(i),z=p.getZ(i),nx=Math.abs(n.getX(i)),ny=Math.abs(n.getY(i)),nz=Math.abs(n.getZ(i));
   uv.push((nz>nx&&nz>ny?x:ny>nx?x:y)*scale,(nz>nx&&nz>ny?y:z)*scale);
  }
  g.setAttribute('uv',new THREE.Float32BufferAttribute(uv,2));mesh.geometry=g;
 };
 const treated=new Set();
 model.traverse(o=>{
  if(!o.isMesh)return;
  const n=o.name,materials=Array.isArray(o.material)?o.material:[o.material];
  const architecture=/^(ROOF__|UPPER__|FRONTWALL__|BACKWALL__)/.test(n);
  o.receiveShadow=true;o.castShadow=!architecture&&!/glazing|carpet|rug/i.test(n);
  for(const m of materials){
   const name=m.name;
   if(/oak|mahogany/i.test(name)&&!m.map){planarUV(o,1.3)}
   if(/linen weave|accent textile|marble|Mica clad/i.test(name)){planarUV(o,/linen/.test(name)?3:1)}
   if(treated.has(m))continue;treated.add(m);
   if(/oak|mahogany/i.test(name)&&!m.map){m.map=wood;m.color.set(/recessed/i.test(name)?'#6d4831':/mahogany/i.test(name)?'#936344':'#c49a6b');m.roughness=.36;m.envMapIntensity=.35}
   if(/photographic carving/.test(name)){m.color.set('#ffe0b1');m.roughness=.86;m.metalness=0;m.envMapIntensity=0}
   if(/linen weave/.test(name)){m.map=linen;m.color.set('#e1d6bb');m.bumpMap=linen;m.bumpScale=.004;m.roughness=.95}
   if(/accent textile/.test(name)){m.map=stripe;m.color.set('#ffffff');m.roughness=.91}
   if(/Crimson silk/.test(name)){m.color.set('#ad8953');m.roughness=.84;m.metalness=0}
   if(/White marble/.test(name)){m.map=marble;m.color.set('#f4efe6');m.roughness=.32}
   if(/Mica clad/.test(name)){m.map=marble;m.color.set('#ad9371');m.roughness=.4}
   if(/Handmade wheat/.test(name)){m.color.set('#d3c6ac');m.roughness=.98}
   if(/Wallpaper pale/.test(name)){m.color.set('#e3d8c0');m.roughness=.92}
   if(/Warm ivory|Window enamel/.test(name)){m.color.set('#e9e2d1');m.roughness=.5}
   if(/gilt|brass/i.test(name)){m.color.set('#bd9855');m.metalness=.7;m.roughness=.3;m.envMapIntensity=.65}
   if(/Lampshade/.test(name)){m.color.set('#fff1cd');m.emissive.set('#f6be75');m.emissiveIntensity=.24;m.roughness=.9}
   if(/Chocolate leather/.test(name)){m.color.set('#765038');m.roughness=.5}
   if(m.map)m.map.anisotropy=Math.min(8,renderer.capabilities.getMaxAnisotropy());
   m.needsUpdate=true;
  }
 });
 const box=(w,h,d,x,y,z,mat)=>{const mesh=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),mat);mesh.position.set(x,y,z);mesh.castShadow=mesh.receiveShadow=true;scene.add(mesh);return mesh};
 // A clipped oak parquet surface restores the floor around the oval carpet.
 const parquet=texture((c,s)=>{
  c.fillStyle='#62422b';c.fillRect(0,0,s,s);
  for(let row=0;row<16;row++)for(let col=-1;col<4;col++){
   const x=col*256+(row%3)*85,y=row*64;
   const t=105+Math.floor(random()*25);c.fillStyle=`rgb(${t+40},${t},${t-43})`;c.fillRect(x+1,y+1,254,62);
   for(let j=0;j<38;j++){const yy=y+random()*62;c.strokeStyle=`rgba(46,24,11,${random()*.17})`;c.beginPath();c.moveTo(x+1,yy);c.bezierCurveTo(x+70,yy-3,x+170,yy+3,x+254,yy);c.stroke()}
  }
 },1024);
 const shape=new THREE.Shape();shape.absellipse(0,0,4.43,5.44,0,Math.PI*2,false,0);
 const geom=new THREE.ShapeGeometry(shape,128),positions=geom.attributes.position,uv=[];
 for(let i=0;i<positions.count;i++)uv.push(positions.getX(i)/4,positions.getY(i)/4);
 geom.setAttribute('uv',new THREE.Float32BufferAttribute(uv,2));
 const floor=new THREE.Mesh(geom,new THREE.MeshStandardMaterial({map:parquet,roughness:.42,metalness:0}));floor.rotation.x=-Math.PI/2;floor.position.y=.008;floor.receiveShadow=true;scene.add(floor);
 // Blue porcelain makes the lamp tables read as deliberate accents.
 const ceramic=new THREE.MeshStandardMaterial({color:0x244d6c,metalness:.08,roughness:.23});
 model.traverse(o=>{if(o.isMesh&&/Glazed_porcelain_lamp|Glazed porcelain lamp/.test(o.name))o.material=ceramic});
 for(const x of [-3.25,3.25]){const l=new THREE.PointLight(0xffd9a1,7,3.5,2);l.position.set(x,1.47,1.95);scene.add(l)}
 const brass=new THREE.MeshStandardMaterial({color:0xb69a59,metalness:.7,roughness:.32});
 // Layered stonework, a fluted surround, and real logs give the hearth depth.
 const stone=new THREE.MeshStandardMaterial({map:marble,color:0xeee8dc,roughness:.46});
 for(const x of [-.98,.98]){
  box(.35,.10,.40,x,.12,5.07,stone);box(.35,.10,.40,x,1.20,5.07,stone);
  for(const off of [-.09,-.045,0,.045,.09])box(.015,.94,.022,x+off,.65,4.918,stone);
 }
 for(const [w,h,z,y] of [[2.28,.055,4.91,1.39],[2.44,.035,4.82,1.50],[1.72,.035,4.89,1.16]])box(w,h,.08,0,y,z,stone);
 const medallion=new THREE.Mesh(new THREE.TorusGeometry(.072,.012,8,40),stone);medallion.position.set(0,1.30,4.907);scene.add(medallion);
 for(let i=0;i<9;i++){const a=i*Math.PI*2/9,p=new THREE.Mesh(new THREE.SphereGeometry(.022,8,6),stone);p.scale.z=.35;p.position.set(Math.cos(a)*.044,1.30+Math.sin(a)*.044,4.895);scene.add(p)}
 const logMat=new THREE.MeshStandardMaterial({color:0x30271f,roughness:1});
 for(let i=0;i<3;i++){const log=new THREE.Mesh(new THREE.CylinderGeometry(.065,.085,.93,12),logMat);log.rotation.z=Math.PI/2;log.rotation.y=(i-1)*.20;log.position.set((i-1)*.06,.19+i*.064,5.09);scene.add(log)}
 box(1.15,.035,.27,0,.14,5.07,new THREE.MeshStandardMaterial({color:0x291b13,emissive:0x8c3513,emissiveIntensity:.25,roughness:1}));
 // Individual curved leaves replace the old rounded placeholder foliage.
 const oldPlant=scene.getObjectByName('Mantel greenery');if(oldPlant)oldPlant.removeFromParent();
 box(1.45,.10,.24,0,1.56,4.99,brass);
 const leafShape=new THREE.Shape();leafShape.moveTo(0,0);leafShape.bezierCurveTo(-.06,.04,-.075,.13,0,.19);leafShape.bezierCurveTo(.075,.13,.06,.04,0,0);
 const leafGeo=new THREE.ShapeGeometry(leafShape,5),lp=leafGeo.attributes.position;
 for(let i=0;i<lp.count;i++)lp.setZ(i,Math.sin(lp.getY(i)/.19*Math.PI)*.026);leafGeo.computeVertexNormals();
 const leaves=new THREE.InstancedMesh(leafGeo,new THREE.MeshStandardMaterial({color:0x42633d,roughness:.86,side:THREE.DoubleSide}),440);
 const transform=new THREE.Object3D();
 for(let i=0;i<440;i++){
  const x=(random()-.5)*1.86,edge=Math.abs(x)/.93;
  transform.position.set(x,1.61+random()*.20*(1-edge*.7),4.80+random()*.30);
  transform.rotation.set(-.3-random()*1.5,random()*Math.PI*2,(random()-.5)*Math.PI*2);transform.scale.setScalar(.50+random()*.65);transform.updateMatrix();leaves.setMatrixAt(i,transform.matrix);
  leaves.setColorAt(i,new THREE.Color().setHSL(.24+random()*.07,.26+random()*.18,.16+random()*.16));
 }
 leaves.castShadow=true;leaves.receiveShadow=true;scene.add(leaves);
 const vase=new THREE.Mesh(new THREE.LatheGeometry([new THREE.Vector2(.09,0),new THREE.Vector2(.12,.02),new THREE.Vector2(.13,.22),new THREE.Vector2(.07,.31),new THREE.Vector2(.08,.35)],32),ceramic);vase.position.set(-3.04,1.115,-3.19);vase.castShadow=true;scene.add(vase);
 const stemMat=new THREE.MeshStandardMaterial({color:0x405b32,roughness:.9});
 const flowerMats=[0xf2e7d2,0xe1b78c,0xb46d64].map(color=>new THREE.MeshStandardMaterial({color,roughness:.8}));
 const petalGeo=new THREE.SphereGeometry(1,10,8);
 for(let i=0;i<17;i++){
  const angle=i*2.4,r=.10+random()*.14,x=-3.04+Math.cos(angle)*r,z=-3.19+Math.sin(angle)*r,y=1.65+random()*.17;
  const curve=new THREE.LineCurve3(new THREE.Vector3(-3.04,1.35,-3.19),new THREE.Vector3(x,y,z));scene.add(new THREE.Mesh(new THREE.TubeGeometry(curve,1,.007,5,false),stemMat));
  for(let j=0;j<6;j++){const a=j*Math.PI/3,p=new THREE.Mesh(petalGeo,flowerMats[i%3]);p.scale.set(.045,.024,.055);p.position.set(x+Math.cos(a)*.035,y+random()*.012,z+Math.sin(a)*.035);p.rotation.y=a;p.castShadow=true;scene.add(p)}
 }
 // Soft contact shadows give heavy furniture weight without a costly postprocess pass.
 const shadow=texture((c,s)=>{const g=c.createRadialGradient(s/2,s/2,0,s/2,s/2,s/2);g.addColorStop(0,'rgba(42,29,14,.33)');g.addColorStop(.5,'rgba(42,29,14,.17)');g.addColorStop(1,'rgba(42,29,14,0)');c.fillStyle=g;c.fillRect(0,0,s,s)},128);
 for(const [x,z,w,h] of [[-2.23,.65,1.6,3],[2.23,.65,1.6,3],[0,-2.54,2.5,1.7],[0,1.05,1.9,1.9],[-3.25,1.95,1,1],[3.25,1.95,1,1]]){
  const s=new THREE.Mesh(new THREE.PlaneGeometry(w,h),new THREE.MeshBasicMaterial({map:shadow,transparent:true,depthWrite:false,toneMapped:false}));s.rotation.x=-Math.PI/2;s.position.set(x,.051,z);scene.add(s);
 }
 fixWindowDetails();
 // Collapse stationary objects into material batches, keeping cutaway sections separate.
 model.updateMatrixWorld(true);
 const batches=new Map(),remove=[];
 model.traverse(o=>{
  if(!o.isMesh||!o.visible||Array.isArray(o.material))return;
  const section=/^(ROOF__|UPPER__|FRONTWALL__)/.exec(o.name)?.[1]||'ROOM__';
  const key=section+o.material.uuid+o.castShadow;
  if(!batches.has(key))batches.set(key,{section,material:o.material,cast:o.castShadow,geometries:[]});
  let g=o.geometry.clone().applyMatrix4(o.matrixWorld);if(g.index)g=g.toNonIndexed();
  for(const name of Object.keys(g.attributes))if(!['position','normal','uv'].includes(name))g.deleteAttribute(name);
  if(!g.attributes.uv)g.setAttribute('uv',new THREE.BufferAttribute(new Float32Array(g.attributes.position.count*2),2));
  batches.get(key).geometries.push(g);remove.push(o);
 });
 for(const o of remove)o.removeFromParent();
 for(const b of batches.values()){
  const merged=mergeGeometries(b.geometries,false);if(!merged)throw new Error('Room batching failed');
  const m=new THREE.Mesh(merged,b.material);m.name=b.section+b.material.name;m.castShadow=b.cast;m.receiveShadow=true;model.add(m);
  b.geometries.forEach(g=>g.dispose());
 }
 shellVisibility();
}

function fixWindowDetails(){
 const originals=new Map(),hide=[];
 model.traverse(o=>{
  if(!o.isMesh)return;
  const name=o.name.replaceAll('_',' '),mats=Array.isArray(o.material)?o.material:[o.material];
  for(const m of mats)originals.set(m.name,m);
  if(/^(US flag|Presidential standard|Gold flag fringe|Ceremonial cord|Flagpole|Finial spread wing|Oil on canvas|Gilt picture frame|Framed desk photograph|Small photograph frame|Picture stand)/.test(name))hide.push(o);
 });
 hide.forEach(o=>{o.visible=false});
 const gold=new THREE.MeshStandardMaterial({color:0xb39860,metalness:.72,roughness:.34});
 const backing=new THREE.MeshStandardMaterial({color:0x392d22,roughness:.82});
 const mount=new THREE.MeshStandardMaterial({color:0xe4dbc8,roughness:.95});
 function box(parent,w,h,d,x,y,z,mat){const m=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),mat);m.position.set(x,y,z);m.castShadow=m.receiveShadow=true;parent.add(m);return m}
 function rectangle(w,h){const s=new THREE.Shape();s.moveTo(-w/2,-h/2);s.lineTo(w/2,-h/2);s.lineTo(w/2,h/2);s.lineTo(-w/2,h/2);s.closePath();return s}
 function frame(parent,w,h,rail,z,mat){
  const shape=rectangle(w+2*rail,h+2*rail),hole=new THREE.Path();
  hole.moveTo(-w/2,-h/2);hole.lineTo(-w/2,h/2);hole.lineTo(w/2,h/2);hole.lineTo(w/2,-h/2);hole.closePath();shape.holes.push(hole);
  const mesh=new THREE.Mesh(new THREE.ExtrudeGeometry(shape,{depth:.023,bevelEnabled:true,bevelSize:.004,bevelThickness:.004,bevelSegments:3,steps:1}),mat);
  mesh.position.z=z;mesh.castShadow=mesh.receiveShadow=true;parent.add(mesh);
 }
 const paintMaterials=[originals.get('Hassam | The Avenue in the Rain, 1917'),originals.get('Rockwell | Working on the Statue of Liberty')];
 for(const [index,x,y,z,w,h] of [[0,-3.12,2.41,-3.51,.68,1.27],[1,3.12,2.52,-3.51,.60,.75]]){
  const group=new THREE.Group();group.name=index?'Window wall Liberty painting':'Window wall Avenue painting';group.position.set(x,y,z);
  // The wall is an ellipse: use its inward normal and leave enough depth for all four rails.
  group.lookAt(x-x/(4.45*4.45),y,z-z/(5.46*5.46));scene.add(group);
  box(group,w+.12,h+.12,.035,0,0,0,backing);
  const material=paintMaterials[index].clone();material.map=material.map.clone();material.map.flipY=false;material.map.needsUpdate=true;material.metalness=0;material.roughness=.87;material.envMapIntensity=.1;
  const artGeometry=new THREE.PlaneGeometry(w,h);const artUV=artGeometry.attributes.uv;for(let i=0;i<artUV.count;i++)artUV.setY(i,1-artUV.getY(i));const art=new THREE.Mesh(artGeometry,material);art.position.z=.024;group.add(art);
  frame(group,w+.01,h+.01,.035,.025,gold);frame(group,w+.085,h+.085,.012,.011,gold);
 }
 // Real photographs replace the procedural green placeholders. Crop in UV space,
 // preserving faces and proportions while retaining the original source images.
 const photoTextures=decorImages.deskPhotos.map(url=>{
  const t=new THREE.TextureLoader().load(url);t.colorSpace=THREE.SRGBColorSpace;
  t.anisotropy=Math.min(8,renderer.capabilities.getMaxAnisotropy());return t;
 });
 const photoOrder=[0,1,2,3,0,2,1,3];
 // Frames sit on the credenza with ivory mounts, deep rails and easel backs.
 for(let i=0;i<8;i++){
  const w=i%3===1?.205:.165,h=i%3===1?.165:.215,g=new THREE.Group();g.name='Credenza photograph '+(i+1);
  g.position.set(-.805+i*.23,.891+h/2,-4.21+(i%2)*.025);g.rotation.x=-.10;g.rotation.y=(i-3.5)*-.025;scene.add(g);
  box(g,w+.025,h+.025,.018,0,0,0,backing);box(g,w,h,.006,0,0,.012,mount);
  const image=photoTextures[photoOrder[i]],pw=w-.025,ph=h-.027,ratio=pw/ph;
  const geometry=new THREE.PlaneGeometry(pw,ph),uv=geometry.attributes.uv;
  for(let n=0;n<uv.count;n++){if(ratio<1)uv.setX(n,.5+(uv.getX(n)-.5)*ratio);else uv.setY(n,.5+(uv.getY(n)-.5)/ratio)}
  const photo=new THREE.Mesh(geometry,new THREE.MeshStandardMaterial({map:image,roughness:.62,metalness:0,emissive:0xffffff,emissiveMap:image,emissiveIntensity:.16}));
  photo.name='Framed memory '+(photoOrder[i]+1);photo.position.z=.018;g.add(photo);
  frame(g,w,h,.012,.017,i%3===0?backing:gold);
  box(g,.035,.015,.10,0,-h/2+.004,-.035,backing);
 }
 // Move the standards clear of the chair and photographs. The two staffs now
 // match their cloth attachment points instead of leaving fabric floating ahead.
 const flagPositions=[-2.10,1.28],poleZ=-4.08,top=3.22,hoist=1.52;
 const textileGold=new THREE.MeshStandardMaterial({color:0xc7a457,roughness:.82,metalness:.08});
 const weaveCanvas=document.createElement('canvas');weaveCanvas.width=weaveCanvas.height=64;
 const wc=weaveCanvas.getContext('2d');wc.fillStyle='#999';wc.fillRect(0,0,64,64);wc.fillStyle='#aaa';
 for(let i=0;i<64;i+=4){wc.fillRect(i,0,1,64);wc.fillRect(0,i,64,1)}
 const weave=new THREE.CanvasTexture(weaveCanvas);weave.wrapS=weave.wrapT=THREE.RepeatWrapping;weave.repeat.set(24,36);
 // Gentle hanging folds: a long, gathered silhouette with a soft scalloped hem.
 // Unlike the old shear, the wave changes through the height of the fabric.
 const points=(kind,u,v)=>{
  const fold=u*Math.PI*6+.38*Math.sin(v*Math.PI),gather=Math.sin(u*Math.PI/2);
  return new THREE.Vector3(flagPositions[kind]+.91*u*(.92+.08*Math.sin(v*Math.PI))+.035*Math.sin(fold)*gather,
   top-hoist*v-.64*Math.pow(u,.78)+.025*Math.sin(fold)*u*v,
   poleZ+.022+.21*Math.sin(fold)*gather+.055*u+.035*Math.sin(v*Math.PI)*u);
 };
 for(let kind=0;kind<2;kind++){
  const poleX=flagPositions[kind],standard=new THREE.Group();standard.name=kind?'Presidential flag staff':'United States flag staff';scene.add(standard);
  const cylinder=(r1,r2,h,y,mat)=>{const m=new THREE.Mesh(new THREE.CylinderGeometry(r1,r2,h,24),mat);m.position.set(poleX,y,poleZ);m.castShadow=m.receiveShadow=true;standard.add(m);return m};
  cylinder(.155,.18,.065,.055,gold);cylinder(.10,.15,.045,.106,gold);cylinder(.020,.032,.14,.19,gold);cylinder(.012,.012,3.04,1.74,gold);
  const finial=new THREE.Mesh(new THREE.SphereGeometry(1,16,12),gold);finial.scale.set(.027,.075,.024);finial.position.set(poleX,3.34,poleZ);standard.add(finial);
  for(const side of [-1,1]){const wing=new THREE.Mesh(new THREE.SphereGeometry(1,12,8),gold);wing.scale.set(.062,.014,.012);wing.rotation.z=side*.35;wing.position.set(poleX+side*.042,3.36,poleZ);standard.add(wing)}
  const src=originals.get(kind===0?'United States flag | woven silk':'Presidential standard | woven silk');
  const material=src.clone();material.map=src.map.clone();material.map.flipY=false;material.map.needsUpdate=true;material.side=THREE.DoubleSide;material.metalness=0;material.roughness=.85;material.envMapIntensity=.25;material.color.set('#ffffff');material.bumpMap=weave;material.bumpScale=.0015;
  const nx=64,ny=72,verts=[],uv=[],indices=[];
  for(let j=0;j<=ny;j++)for(let i=0;i<=nx;i++){const p=points(kind,i/nx,j/ny);verts.push(p.x,p.y,p.z);uv.push(i/nx,j/ny)}
  for(let j=0;j<ny;j++)for(let i=0;i<nx;i++){const a=j*(nx+1)+i,b=a+1,c=a+nx+1,d=c+1;indices.push(a,c,b,b,c,d)}
  const geometry=new THREE.BufferGeometry();geometry.setAttribute('position',new THREE.Float32BufferAttribute(verts,3));geometry.setAttribute('uv',new THREE.Float32BufferAttribute(uv,2));geometry.setIndex(indices);geometry.computeVertexNormals();
  const flag=new THREE.Mesh(geometry,material);flag.name=kind?'Draped presidential standard':'Draped United States flag';flag.castShadow=true;flag.receiveShadow=true;scene.add(flag);
  const tube=(pts,r,mat)=>{const path=new THREE.CatmullRomCurve3(pts),mesh=new THREE.Mesh(new THREE.TubeGeometry(path,Math.max(8,pts.length*2),r,5,false),mat);mesh.castShadow=true;scene.add(mesh)};
  // Tailored gold binding tracks the actual cloth edge, including the lower hem.
  const hem=[];for(let j=0;j<=72;j++)hem.push(points(kind,1,j/72));for(let i=63;i>=0;i--)hem.push(points(kind,i/64,1));tube(hem,.005,textileGold);
  const fringe=new THREE.InstancedMesh(new THREE.CylinderGeometry(.0015,.002,.036,5),textileGold,64),transform=new THREE.Object3D();
  fringe.name=kind?'Presidential standard gold fringe':'United States flag gold fringe';
  for(let i=0;i<64;i++){const p=i<40?points(kind,1,i/39):points(kind,(i-40)/23,1);transform.position.copy(p);transform.position.y-=.018;transform.rotation.z=.12*Math.sin(i*2.4);transform.updateMatrix();fringe.setMatrixAt(i,transform.matrix)}
  scene.add(fringe);
  for(const v of [0,.92]){const y=top-hoist*v;tube([new THREE.Vector3(poleX-.014,y,poleZ),new THREE.Vector3(poleX+.018,y,poleZ+.029),points(kind,0,v)],.004,textileGold)}
  tube([points(kind,0,0),new THREE.Vector3(poleX+.095,3.01,poleZ+.07),new THREE.Vector3(poleX+.14,2.93,poleZ+.08)],.004,textileGold);
 }
}
