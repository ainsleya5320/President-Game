// Clearance and connectivity acceptance for the Blender-authored building.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const nav=JSON.parse(fs.readFileSync(path.join(__dirname,'../../..','west-wing/navigation.json'),'utf8'));
const source=fs.readFileSync(path.join(__dirname,'west-wing-level.js'),'utf8');
const start=source.indexOf('function westWingCanWalk'),end=source.indexOf('function westWingZone');
const canWalk=vm.runInNewContext(source.slice(start,end)+'\nwestWingCanWalk',{westWingNav:nav,Math});
const step=.20,origin=[-18,-11],width=276,height=231,key=(i,j)=>j*width+i;
const walk=(x,y)=>canWalk(x,-y),index=(x,y)=>[Math.round((x-origin[0])/step),Math.round((y-origin[1])/step)];
const [sx,sy]=index(...nav.spawn),queue=[key(sx,sy)],visited=new Set(queue),parent=new Map(),valid=new Map();
function available(i,j){if(i<0||j<0||i>=width||j>=height)return false;const k=key(i,j);if(!valid.has(k))valid.set(k,walk(origin[0]+i*step,origin[1]+j*step));return valid.get(k)}
for(let n=0;n<queue.length;n++){
 const p=queue[n],i=p%width,j=Math.floor(p/width);
 for(const [di,dj] of [[1,0],[-1,0],[0,1],[0,-1]]){const ni=i+di,nj=j+dj,k=key(ni,nj);if(!visited.has(k)&&available(ni,nj)&&walk(origin[0]+(i+di/2)*step,origin[1]+(j+dj/2)*step)){visited.add(k);parent.set(k,p);queue.push(k)}}
}
for(const zone of nav.zones){assert.ok(walk(...zone.spawn),zone.name+' has a clear player start');assert.ok(visited.has(key(...index(...zone.spawn))),zone.name+' is reachable on foot from the Oval doorway');}
for(const point of [[4.5,10.5],[1.5,8.5],[-7,8.5],[-1.8,1.5],[4.5,22.5],[13,11],[17,6.6],[25,2.5]])assert.ok(visited.has(key(...index(...point))),'Door/path reachable: '+point);
for(const point of [[8.75,13.4],[-2.75,8.6],[4.5,15],[-12,8],[9.25,32],[18.2,-2],[29.5,2],[40,0]])assert.equal(walk(...point),false,'Solid/outside blocks movement: '+point);
// Full press center aisle remains clear up to the stage.
for(let y=21.2;y<30.4;y+=.2)assert.ok(walk(8.75,y),'Press center aisle at '+y);
console.log('West Wing navigation passed: all 8 destinations and doorways reachable from the Oval, press aisle clear, tables/walls/stage/garden obstacles blocked. '+visited.size+' reachable samples.');
