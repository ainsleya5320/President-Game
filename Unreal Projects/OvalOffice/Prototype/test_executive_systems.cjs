const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const Sim=vm.runInNewContext(['executive-systems.js','simulation-core.js'].map(f=>fs.readFileSync(__dirname+'/'+f,'utf8')).join('\n')+'\nFourYearsSim'),E=Sim.executive;
function turn(s){assert.equal(Sim.choose(s,1),true);return Sim.advance(s)}
const s=Sim.fresh();
assert.equal(Sim.changePolicy(s,'housing',4).ok,false,'major funding cannot bypass Congress');
assert.equal(E.platform(s,['housing','housing','trust'],'housing').ok,false);
assert.equal(E.platform(s,['housing','health','trust'],'housing').ok,true);
assert.equal(E.platform(s,['housing','health','energy'],'grid').ok,false,'platform is a commitment');
assert.equal(E.propose(s,'housing').ok,true);assert.equal(E.propose(s,'grid').ok,false);
assert.ok(E.votes(s,E.activeBill(s)).reduce((n,b)=>n+b.yes,0)<51);
assert.equal(E.amend(s,'local').ok,true);assert.equal(E.amend(s,'audit').ok,true);
assert.ok(E.votes(s,E.activeBill(s)).reduce((n,b)=>n+b.yes,0)>=51);
assert.equal(E.amend(s,'audit').ok,false,'no duplicate amendment');
assert.equal(E.spending(s),0,'draft amendments cannot spend money');
assert.equal(E.vote(s).ok,true);assert.equal(s.executive.bills[0].status,'passed');
assert.equal(s.levels.housing,4);assert.equal(E.spending(s),4);
assert.equal(E.vote(s).ok,false,'passed bill cannot be voted repeatedly');
const initialLocal=s.executive.regions.lakes.housing;
for(let i=0;i<4;i++)turn(s);
assert.equal(s.executive.bills[0].progress,100);
assert.ok(s.executive.regions.coast.news.some(n=>n.text.includes('delivered')));
assert.ok(s.executive.regions.lakes.housing>initialLocal,'construction improves a region outside the storm path');
assert.equal(s.executive.crises.some(c=>c.id==='contracts'),false,'oversight prevents construction scandal');

const fail=Sim.fresh();E.propose(fail,'housing');E.vote(fail);
assert.equal(fail.executive.bills[0].status,'failed');assert.equal(fail.levels.housing,2);
assert.equal(E.vote(fail).ok,false,'one floor vote per quarter');
turn(fail);E.amend(fail,'local');E.amend(fail,'audit');E.amend(fail,'revenue');
assert.equal(E.vote(fail).ok,true);assert.equal(fail.executive.bills[0].status,'passed');assert.equal(fail.levels.tax,3);

const weather=Sim.fresh();weather.metrics.energy=40;
turn(weather);turn(weather);
let storm=weather.executive.crises.find(c=>c.id==='storm');assert.equal(storm.stage,'warning');
assert.equal(E.respondCrisis(weather,'storm','prepare').ok,true);
assert.equal(E.respondCrisis(weather,'storm','prepare').ok,false,'cannot buy preparation repeatedly');
turn(weather);assert.equal(storm.stage,'landfall');assert.ok(storm.damage<=4);
E.respondCrisis(weather,'storm','full');turn(weather);assert.equal(storm.stage,'investigation');
E.respondCrisis(weather,'storm','publish');turn(weather);assert.equal(storm.stage,'resolved');

const cabinet=Sim.fresh();cabinet.relationships.chen=15;turn(cabinet);turn(cabinet);
assert.equal(cabinet.executive.cabinet.chen.status,'resigned');
assert.equal(Sim.meeting(cabinet,'chen','listen').ok,false);
assert.ok(E.staffFactor(cabinet,'housing')<.7);
assert.equal(E.teamAction(cabinet,'chen','replace').ok,true);
assert.equal(Sim.people(cabinet).find(p=>p.id==='chen').name,'Dr. Nia Patel');
assert.equal(cabinet.relationships.chen,50);
assert.equal(E.teamAction(cabinet,'chen','replace').ok,false);

const media=Sim.fresh();assert.equal(E.startEncounter(media,'debate').ok,false);
assert.equal(E.startEncounter(media,'press').ok,true);E.answerEncounter(media,'timeline');
assert.match(E.encounterOptions(media,media.executive.encounters[0]).question,/missed deadlines/);
assert.equal(E.answerEncounter(media,'transparent').ok,true);assert.equal(E.answerEncounter(media,'transparent').ok,false);
assert.equal(E.startEncounter(media,'press').ok,false);assert.equal(Sim.availableSlots(media),1);
media.quarter=7;assert.equal(E.startEncounter(media,'debate').ok,true);E.answerEncounter(media,'own');E.answerEncounter(media,'listen');

const country=Sim.fresh();assert.equal(E.regionalVisit(country,'lakes').ok,false);
country.location='aircraft';const before=country.executive.regions.lakes.goodwill;
assert.equal(E.regionalVisit(country,'lakes').ok,true);assert.ok(country.executive.regions.lakes.goodwill>before);
assert.equal(E.regionalVisit(country,'lakes').ok,false);
const old=Sim.fresh();old.version=3;old.quarter=5;old.location='aircraft';delete old.executive;
const migrated=Sim.migrate(JSON.parse(JSON.stringify(old)));assert.equal(migrated.version,4);assert.equal(migrated.quarter,5);assert.equal(migrated.location,'aircraft');assert.ok(migrated.executive.regions.lakes);
const funded=Sim.fresh();funded.version=3;funded.levels.housing=4;funded.implemented.housing=3;delete funded.executive;
const preserved=Sim.migrate(funded);assert.equal(preserved.executive.bills[0].status,'passed');assert.equal(preserved.executive.bills[0].progress,50);assert.equal(preserved.debt,72,'migration never charges for existing authority');
Sim.changePolicy(preserved,'housing',3);assert.equal(Sim.changePolicy(preserved,'housing',4).ok,true);
const tie=Sim.fresh();for(const r of E.REGIONS)tie.executive.regions[r.id].goodwill=['coast','plains'].includes(r.id)?15:-15;
assert.equal(E.election(tie,50).points,50);assert.equal(E.election(tie,50).won,false,'a tied electoral count is not a majority');
const term=Sim.fresh();for(let i=0;i<16;i++)turn(term);assert.ok(term.executive.finalElection);assert.equal(term.executive.finalElection.regions.length,4);assert.equal(E.propose(term,'housing').ok,false);assert.equal(E.legacy(term).goals.length,3);
console.log('Executive systems passed: congressional votes, delivery, fiscal tradeoffs, storm and scandal branches, cabinet replacement, encounters, regional visits, migration and final legacy.');
