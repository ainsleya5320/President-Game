const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),{loadSimulation}=require('./load_simulation.cjs');
const Sim=loadSimulation();
const quarter=(s,choice=1)=>{assert.equal(Sim.choose(s,choice),true);return Sim.advance(s);};

// A seeded population of individual voters whose groups overlap.
const a=Sim.fresh(11),view=Sim.electorate(a);
assert.deepEqual(Sim.electorate(Sim.fresh(11)),view,'the same seed builds the same electorate');
assert.ok(view.overlap>1.5,'voters belong to several groups');
for(const g of view.groups)assert.ok(Math.abs(g.share-Sim.GROUPS.find(x=>x.id===g.id).share)<=4,`${g.id} starts near its data share`);
assert.ok(view.groups.reduce((n,g)=>n+g.share,0)>100);
assert.ok(view.poll>=45&&view.poll<=58,'a new term starts competitive');
const polls=new Set([1,2,3,4,5,6].map(seed=>Sim.poll(Sim.fresh(seed))));assert.ok(polls.size>1,'each term surveys a different country');

// Membership follows conditions: failing public health creates more health-care voters.
const sick=Sim.fresh(11);sick.metrics.health=25;
assert.ok(Sim.groups(sick).find(g=>g.id==='health').share>view.groups.find(g=>g.id==='health').share);

// The poll responds to group moods through individual voters.
const clinics=Sim.fresh(11),control=Sim.fresh(11);
for(const s of [clinics,control])s.deck.current='research-breakthrough';
Sim.changePolicy(clinics,'clinic',4);quarter(clinics);quarter(control);
const health=s=>Sim.groups(s).find(g=>g.id==='health');
assert.ok(health(clinics).support>health(control).support,'health-care voters warm to clinics');
const boosted=Sim.fresh(11);for(const g of Sim.GROUPS)boosted.goodwill[g.id]=8;assert.ok(Sim.poll(boosted)>Sim.poll(a));

// Radicalization: a group kept far below the threshold escalates, and the report says so.
const angry=Sim.fresh(11);Object.assign(angry.levels,{fossil:4,carbon:0,renewables:0});angry.metrics.climate=15;angry.metrics.liberty=25;
const seen=[];for(let q=0;q<6;q++)seen.push(...quarter(angry).changes);
assert.ok(angry.unrest.anger.climate>=25,'climate advocates grow angry');
assert.ok(seen.some(x=>x.startsWith('Unrest rising. Climate advocates')));
assert.ok(Sim.unrest(angry).some(g=>g.id==='climate'&&g.stage));
assert.ok(angry.history.some(h=>h.type==='unrest'));

// Stages carry costs: disruption drags on conditions and a tax revolt costs revenue.
const calm=Sim.fresh(11),riot=Sim.fresh(11);riot.unrest.anger.workers=60;riot.unrest.anger.fiscal=60;
assert.equal(Sim.unrestStage(60).name,'Disruption');assert.equal(Sim.unrestStage(10),null);
assert.ok(Sim.budget(riot).revenue<Sim.budget(calm).revenue,'a tax revolt cuts revenue');
for(const s of [calm,riot])s.deck.current='research-breakthrough';quarter(calm);quarter(riot);
assert.ok(riot.metrics.growth<calm.metrics.growth,'strikes slow growth');
const cooled=Sim.fresh(11);cooled.unrest.anger.workers=30;cooled.deck.current='research-breakthrough';quarter(cooled);
assert.ok(cooled.unrest.anger.workers<30,'anger fades once a group is no longer furious');

// Crisis responses can calm the angriest group, or inflame it.
const talk=Sim.fresh(11);talk.unrest.anger.students=40;talk.deck.current='protests-at-the-gates';
assert.equal(Sim.eventEligible(talk,Sim.EVENTS.find(e=>e.id==='protests-at-the-gates')),true);
assert.equal(Sim.eventEligible(Sim.fresh(11),Sim.EVENTS.find(e=>e.id==='protests-at-the-gates')),false);
Sim.choose(talk,0);assert.equal(talk.unrest.anger.students,15);
const hard=Sim.fresh(11);hard.unrest.anger.students=40;hard.deck.current='protests-at-the-gates';Sim.choose(hard,2);assert.equal(hard.unrest.anger.students,50);

// Extremists create a seeded risk of an attempt on the president; security policy lowers it.
function incidents(police,surveillance){let n=0;for(let seed=1;seed<=200;seed++){const s=Sim.fresh(seed);s.levels.police=police;s.levels.surveillance=surveillance;s.unrest.anger.climate=100;s.unrest.anger.workers=100;s.deck.current='research-breakthrough';s.metrics.climate=5;Sim.choose(s,1);Sim.advance(s);n+=s.unrest.incidents.length;}return n;}
const exposed=incidents(0,0),guarded=incidents(4,4);
assert.ok(exposed>guarded&&guarded>0,`security lowers the odds (${exposed} vs ${guarded} per 200 terms)`);
const hit=(()=>{for(let seed=1;seed<=200;seed++){const s=Sim.fresh(seed);s.unrest.anger.climate=100;s.deck.current='research-breakthrough';s.metrics.climate=5;Sim.choose(s,1);const r=Sim.advance(s);if(s.unrest.incidents.length)return {s,r};}})();
assert.ok(hit,'an extremist threat eventually produces an incident');
assert.ok(hit.r.changes.some(x=>/foiled|motorcade/.test(x))&&hit.s.history.some(h=>h.type==='security'));

// Saves from before the electorate gain unrest tracking.
const old=Sim.fresh(11);old.version=5;delete old.unrest;
const migrated=Sim.migrate(JSON.parse(JSON.stringify(old)));assert.equal(migrated.version,6);assert.equal(JSON.stringify(migrated.unrest),JSON.stringify({anger:{},incidents:[]}));
quarter(migrated);

// Electorate data is validated like the rest.
const data=Object.fromEntries(fs.readdirSync(path.join(__dirname,'data')).filter(f=>f.endsWith('.json')).map(f=>[path.basename(f,'.json'),JSON.parse(fs.readFileSync(path.join(__dirname,'data',f),'utf8'))]));
data.groups[0].traits.religion={devout:2};data.electorate.traits.age.young=.9;data.unrest.attempt.security.army=.1;
assert.throws(()=>loadSimulation(data),e=>/religion/.test(e.message)&&/trait age/.test(e.message)&&/army/.test(e.message));
console.log(`Electorate checks passed: overlapping voter groups, shifting membership, voter-level polling, radicalization stages and costs, calming responses, security odds (${exposed} vs ${guarded}), migration and validation.`);
