const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const source=fs.readFileSync(__dirname+'/simulation-core.js','utf8');
const Sim=vm.runInNewContext(source+'\nFourYearsSim');

const initial=Sim.fresh();
assert.equal(Sim.P.length,26);
assert.equal(Sim.GROUPS.reduce((n,g)=>n+g.share,0),100);
assert.equal(Sim.AGENDA.length,16);
assert.equal(initial.quarter,0);

const fiscal=Sim.fresh(),baseBudget=Sim.budget(fiscal);
assert.equal(Sim.changePolicy(fiscal,'tax',4).ok,true);
assert.ok(Sim.budget(fiscal).revenue>baseBudget.revenue);
assert.equal(fiscal.capital,8);
assert.equal(Sim.changePolicy(fiscal,'tax',4).ok,false);
const taxCut=Sim.fresh();Sim.changePolicy(taxCut,'tax',0);
assert.ok(Sim.budget(taxCut).revenue<baseBudget.revenue,'tax cuts should reduce revenue');
const clinicCut=Sim.fresh();Sim.changePolicy(clinicCut,'clinic',0);
assert.ok(Sim.budget(clinicCut).spending<baseBudget.spending,'program cuts should reduce spending');

const clinic=Sim.fresh(),control=Sim.fresh();
assert.equal(Sim.changePolicy(clinic,'clinic',4).ok,true);
assert.equal(Sim.choose(clinic,0),true);
assert.equal(Sim.choose(control,0),true);
Sim.advance(clinic);Sim.advance(control);
assert.ok(clinic.implemented.clinic<4,'implementation must ramp');
assert.ok(clinic.metrics.health>control.metrics.health,'clinics should improve health');
assert.ok(Sim.budget(clinic).spending>Sim.budget(control).spending,'clinics must cost money');
assert.ok(Sim.groups(clinic).find(g=>g.id==='health').approval>Sim.groups(control).find(g=>g.id==='health').approval);

const surveillance=Sim.fresh();
Sim.changePolicy(surveillance,'surveillance',4);Sim.choose(surveillance,0);Sim.advance(surveillance);
assert.ok(surveillance.metrics.safety>initial.metrics.safety);
assert.ok(surveillance.metrics.liberty<initial.metrics.liberty);
assert.ok(Sim.groups(surveillance).find(g=>g.id==='civil').approval<Sim.groups(initial).find(g=>g.id==='civil').approval);

const legacy=Sim.migrate({month:7,trust:63,capital:5,fiscal:41,location:'aircraft',history:[{month:0,event:'Old choice',choice:'Acted'}]});
assert.equal(legacy.location,'aircraft');
assert.equal(legacy.quarter,2);
assert.equal(legacy.legacy.history.length,1);

const term=Sim.fresh();
for(let q=0;q<16;q++){
 assert.equal(Sim.choose(term,0),true);
 const report=Sim.advance(term);
 assert.equal(report.quarter,q+1);
 if(q===7)assert.equal(report.election.type,'midterm');
 if(q===15)assert.equal(report.election.type,'general');
}
assert.equal(term.ended,true);
assert.equal(term.polls.length,16);
assert.equal(Sim.choose(term,0),false);
console.log('Simulation checks passed: policy tradeoffs, delayed implementation, voters, budget, save migration, and four-year elections.');
