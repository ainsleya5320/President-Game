const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),{loadSimulation}=require('./load_simulation.cjs');
const Sim=loadSimulation();
const term=seed=>{const s=Sim.fresh(seed),ids=[];while(!s.ended){ids.push(Sim.currentEvent(s).id);assert.equal(Sim.choose(s,1),true);Sim.advance(s);}return ids;};

// Reproducible per seed, no repeats, fixed events in place, and different seeds produce different terms.
const first=term(6137);
assert.deepEqual(term(6137),first,'the same seed replays the same term');
assert.equal(first.length,16);assert.equal(new Set(first).size,16,'no event repeats within a term');
assert.equal(first[8],'the-midterm-aftermath');assert.equal(first[15],'the-final-address');
assert.ok([1,2,3,4,5].some(seed=>term(seed).join()!==first.join()),'other seeds draw other events');
assert.equal(Sim.fresh().seed,6137,'rules tests stay deterministic by default');
assert.ok(Number.isInteger(Sim.newSeed()));

// Conditions: crisis events only appear while their situation is active.
const rescue=Sim.EVENTS.find(e=>e.id==='recession-rescue'),calm=Sim.fresh();
assert.equal(Sim.eventEligible(calm,rescue),false);
calm.situations=['Recession'];assert.equal(Sim.eventEligible(calm,rescue),true);
const budgetEvent=Sim.EVENTS.find(e=>e.id==='a-difficult-budget');calm.quarter=4;calm.debt=80;
assert.equal(Sim.eventEligible(calm,budgetEvent),false);calm.debt=90;assert.equal(Sim.eventEligible(calm,budgetEvent),true);

// Situations start at their trigger and clear only after recovering past clearsAt.
const h=Sim.fresh();h.metrics.growth=37.5;h.situations=Sim.updateSituations(h);assert.ok(h.situations.includes('Recession'));
h.metrics.growth=40;h.situations=Sim.updateSituations(h);assert.ok(h.situations.includes('Recession'),'recovery below clearsAt keeps it active');
h.metrics.growth=42;h.situations=Sim.updateSituations(h);assert.ok(!h.situations.includes('Recession'));
const d=Sim.fresh();d.debt=141;assert.ok(Sim.updateSituations(d).includes('Debt warning'),'debt can trigger situations');

// Active situations change voters, the budget, capital and conditions.
const base=Sim.fresh(),hit=Sim.fresh();hit.situations=['Recession'];
assert.ok(Sim.groups(hit).find(g=>g.id==='workers').approval<Sim.groups(base).find(g=>g.id==='workers').approval);
assert.ok(Sim.budget(hit).revenue<Sim.budget(base).revenue,'a recession shrinks receipts');
for(const s of [base,hit]){s.deck.current='research-breakthrough';Sim.choose(s,1);}
Sim.advance(base);Sim.advance(hit);
assert.ok(hit.metrics.jobs<base.metrics.jobs,'a recession costs jobs');
const doubt=Sim.fresh(),steady=Sim.fresh();doubt.metrics.trust=30;steady.metrics.trust=30;doubt.situations=['Trust crisis'];
for(const s of [doubt,steady]){s.deck.current='research-breakthrough';Sim.choose(s,1);}
Sim.advance(doubt);Sim.advance(steady);
assert.ok(doubt.situations.includes('Trust crisis')&&!steady.situations.includes('Trust crisis'),'an active crisis outlasts a fresh dip to the same level');
assert.ok(doubt.capital<steady.capital,'a trust crisis costs political capital');
const onset=Sim.fresh();onset.metrics.growth=20;onset.deck.current='research-breakthrough';Sim.choose(onset,1);
assert.ok(Sim.advance(onset).changes.includes('New situation: Recession'),'the quarterly report announces new situations');

// Negative costs are savings.
const saver=Sim.fresh();saver.deck.current='credit-rating-warning';const debt=saver.debt;Sim.choose(saver,0);assert.ok(saver.debt<debt);

// Saves from before the deck keep the event already on the desk, then continue with unused events.
const old=Sim.fresh();old.version=4;old.quarter=5;old.agendaChoice=1;delete old.deck;
const migrated=Sim.migrate(JSON.parse(JSON.stringify(old)));
assert.equal(migrated.version,6);assert.equal(Sim.currentEvent(migrated).id,'housing-cost-surge');
assert.equal(migrated.deck.used.length,6);const shown=[...migrated.deck.used];Sim.advance(migrated);
assert.ok(!shown.includes(Sim.currentEvent(migrated).id),'events already seen are not drawn again');
assert.equal(new Set(migrated.deck.used).size,migrated.deck.used.length);

// A save whose event was later removed from the data draws a replacement instead of breaking.
const orphan=Sim.fresh();orphan.deck.current='retired-event';orphan.agendaChoice=0;
const repaired=Sim.migrate(JSON.parse(JSON.stringify(orphan)));assert.ok(Sim.currentEvent(repaired));assert.equal(repaired.agendaChoice,null);

// Broken data fails loudly with a readable list of problems.
const data=Object.fromEntries(fs.readdirSync(path.join(__dirname,'data')).filter(f=>f.endsWith('.json')).map(f=>[path.basename(f,'.json'),JSON.parse(fs.readFileSync(path.join(__dirname,'data',f),'utf8'))]));
data.events[0].choices[0].effects.happiness=3;data.events[1].when={situations:['Alien invasion']};data.situations[0].clearsAt=30;
assert.throws(()=>loadSimulation(data),e=>/happiness/.test(e.message)&&/Alien invasion/.test(e.message)&&/clearsAt/.test(e.message));
console.log('Event deck checks passed: seeded draws, conditions, fixed quarters, situation hysteresis and effects, savings, save migration and data validation.');
