// Run with Node. Exercise the browser's actual collision code against the
// independently generated Blender routes, including points between grid samples.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../../../..');
const source=fs.readFileSync(path.join(__dirname,'aircraft-level.js'),'utf8');
const footprints=JSON.parse(fs.readFileSync(path.join(root,'air-force-one/collision_footprints.json'),'utf8')).map(p=>p.polygon);
const context={clamp:(x,a,b)=>Math.max(a,Math.min(b,x))};vm.createContext(context);
vm.runInContext(source.slice(source.indexOf('const aircraftFootprints='),source.indexOf('async function loadAircraft')).replace('__AIRCRAFT_COLLISION__',JSON.stringify(footprints)),context);
const walk=(x,y)=>vm.runInContext(`aircraftCanWalk(${x},${-y})`,context);
const routes=JSON.parse(fs.readFileSync(path.join(root,'air-force-one/walkability_report.json'),'utf8')).paths;
let samples=0;
for(const [name,points] of Object.entries(routes)){
 for(let i=0;i<points.length;i++){
  const a=points[i],b=points[Math.min(i+1,points.length-1)];
  for(let t=0;t<=1;t+=.25){const x=a[0]+(b[0]-a[0])*t,y=a[1]+(b[1]-a[1])*t;assert(walk(x,y),`${name}: blocked at ${x},${y}`);samples++}
 }
}
for(const p of [[2.77,1],[.76,3.6],[1.5,6.4]])assert(walk(...p),'Room shortcut must land in clear space');
for(const p of [[3.55,6],[2.77,-.2],[-4,4],[0,14],[-.58,9.02]])assert(!walk(...p),'Walls, exterior and table must block walking');
console.log(JSON.stringify({routes:Object.keys(routes).length,samples,roomSpawns:3,blockedPoints:5,status:'passed'}));
