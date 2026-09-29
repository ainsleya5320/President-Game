const assert=require('node:assert/strict'),{loadSimulation}=require('./load_simulation.cjs');
const Sim=loadSimulation(),D=Sim.countyData,clone=x=>JSON.parse(JSON.stringify(x));
assert.equal(D.counties.length,3143);assert.equal(new Set(D.counties.map(c=>c.id)).size,3143);assert.equal(D.states.length,51);assert.equal(D.states.reduce((n,s)=>n+s.ev,0),538);
assert.equal(D.counties.filter(c=>c.electionSource!=='county'&&c.state!=='AK').map(c=>c.id).join(','),'15005');
for(const c of D.counties){assert.ok(c.population>0&&c.adults>0);assert.ok(c.dem2024>0&&c.dem2024<100);assert.ok(Math.abs(Object.values(c.cultures).reduce((a,b)=>a+b,0)-1)<1e-9);assert.ok(c.college>=0&&c.college<=1&&c.homeowner>=0&&c.homeowner<=1);}
// Official state returns and Maine/Nebraska district rules reproduce the 2024 result.
let dem=0;for(const st of D.states){const rows=D.counties.filter(c=>c.state===st.id),votes=rows.reduce((n,c)=>n+c.ballots,0),d=rows.reduce((n,c)=>n+c.ballots*c.dem2024/100,0);assert.ok(Math.abs(d/votes*100-st.dem2024)<.00001,st.id+' calibrated vote');if(st.dem2024>50)dem+=['ME','NE'].includes(st.id)?2:st.ev;}
for(const d of D.districts)if(d.dem2024>50)dem++;
assert.equal(dem,226);assert.equal(538-dem,312);
const s=Sim.fresh(33),before=clone(s);assert.equal(Sim.chooseParty(s,'democrat').ok,true);assert.equal(Sim.chooseParty(s,'republican').ok,false);assert.deepEqual(clone(s.metrics),before.metrics);assert.equal(s.quarter,before.quarter);
let v=Sim.voterAtlas(s);assert.equal(v.points+v.opponent+v.tossup,538);assert.equal(v.units.length,56);assert.equal(v.counties.length,3143);assert.equal(v.nations.length,14);
for(const c of v.counties){assert.equal(c.cohorts.length,8);assert.ok(Math.abs(c.cohorts.reduce((n,g)=>n+g.share,0)-100)<.01);assert.ok(c.approval>=0&&c.approval<=100&&c.turnout>0&&c.turnout<=100);}
const county=(v,id)=>v.counties.find(c=>c.id===id);
assert.ok(county(v,'06075').support>county(v,'06029').support+20,'local partisan differences survive within California');
const bad=clone(s);bad.metrics.prices=15;bad.metrics.jobs=20;assert.ok(Sim.voterAtlas(bad).support<v.support-2,'economic failure affects votes');
const help=clone(s);help.implemented.clinic=4;help.implemented.insurance=4;help.metrics.health=70;const vh=Sim.voterAtlas(help);assert.ok(vh.approval>v.approval);assert.ok(Math.abs((county(vh,'06075').approval-county(v,'06075').approval)-(county(vh,'01001').approval-county(v,'01001').approval))>.01,'cultural and demographic policy responses vary');
const r=clone(before);Sim.chooseParty(r,'republican');const rv=Sim.voterAtlas(r);assert.equal(rv.approval,v.approval,'party identity is distinct from job approval');assert.ok(county(rv,'06075').support<county(rv,'06029').support);
const current=clone(s),capital=current.capital;assert.equal(Sim.organize(current,'17031').ok,true);assert.equal(current.capital,capital-2);assert.equal(Sim.organize(current,'06075').ok,false);assert.ok(county(Sim.voterAtlas(current),'17031').support>county(v,'17031').support);assert.ok(county(Sim.voterAtlas(current),'17031').turnout>county(v,'17031').turnout);
current.quarter++;assert.ok(county(Sim.voterAtlas(current),'17031').field<2.5,'organizing fades');
const old=Sim.migrate(clone(s));assert.equal(old.electoral.party,'democrat');assert.equal(Sim.chooseParty(old,'republican').ok,false);
const closed=clone(s);closed.agendaChoice=0;assert.equal(Sim.organize(closed,'17031').ok,false);
const noMoney=clone(s);noMoney.capital=1;assert.equal(Sim.organize(noMoney,'17031').ok,false);
const finish=clone(s);finish.ended=true;assert.equal(Sim.organize(finish,'17031').ok,false);
console.log('County electorate: geography, data joins, 2024 calibration (226/312), 538 electors, 25,144 overlapping cohorts, party choice, policy response, turnout, organizing, migration and action guards passed.');
