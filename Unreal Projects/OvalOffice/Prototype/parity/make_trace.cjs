// Plays seeded bot terms through every simulation action and records each result plus the full state after
// every quarter. The Unreal C++ core replays the same actions and must reproduce this trace exactly.
const fs=require('node:fs'),path=require('node:path'),{loadSimulation}=require('../load_simulation.cjs');
const Sim=loadSimulation(),E=Sim.executive,clone=x=>JSON.parse(JSON.stringify(x));
function bot(seed){let a=seed>>>0;return ()=>{a=(a+0x6d2b79f5)>>>0;let t=a;t=Math.imul(t^(t>>>15),t|1);t^=t+Math.imul(t^(t>>>7),t|61);return ((t^(t>>>14))>>>0)/4294967296;};}
const pick=(r,list)=>list[Math.floor(r()*list.length)];
const ACTIONS={
 chooseParty:(s,a)=>Sim.chooseParty(s,a[0]),organize:(s,a)=>Sim.organize(s,a[0]),
 changePolicy:(s,a)=>Sim.changePolicy(s,a[0],a[1]),meeting:(s,a)=>Sim.meeting(s,a[0],a[1]),campaignTrip:(s,a)=>Sim.campaignTrip(s,a[0],a[1],a[2]),
 platform:(s,a)=>E.platform(s,a[0],a[1]),propose:(s,a)=>E.propose(s,a[0]),amend:(s,a)=>E.amend(s,a[0]),lobby:(s,a)=>E.lobby(s,a[0]),vote:s=>E.vote(s),
 regionalVisit:(s,a)=>E.regionalVisit(s,a[0]),teamAction:(s,a)=>E.teamAction(s,a[0],a[1]),respondCrisis:(s,a)=>E.respondCrisis(s,a[0],a[1]),
 startEncounter:(s,a)=>E.startEncounter(s,a[0]),answerEncounter:(s,a)=>E.answerEncounter(s,a[0]),
 setLocation:(s,a)=>{s.location=a[0];return {ok:true};},choose:(s,a)=>Sim.choose(s,a[0]),advance:s=>Sim.advance(s)
};
// Derived values the UI reads; they are compared as well as the saved state.
function derived(s){const e=Sim.electorate(s),ev=Sim.currentEvent(s);return {poll:e.poll,overlap:e.overlap,groups:e.groups.map(g=>({id:g.id,approval:g.approval,turnout:g.turnout,share:g.share,support:g.support,anger:g.anger,stage:g.stage})),budget:Sim.budget(s),election:E.election(s,e.poll),event:ev?ev.id:null,slots:Sim.availableSlots(s),legacy:clone(E.legacy(s))};}
function botAction(r,s){
 if(!s.electoral?.party)return ['chooseParty',[s.seed%2?'democrat':'republican']];
 if(r()<.12)return ['organize',[pick(r,['17031','06075','12086','bad'])]];
 // Mostly play sensibly so every success path is exercised; sometimes try something invalid on purpose.
 const live=s.executive.encounters.find(e=>e.quarter===s.quarter&&!e.done),crisis=s.executive.crises.find(c=>c.stage!=='resolved'&&!c.response);
 if(live&&r()<.8)return ['answerEncounter',[pick(r,E.encounterOptions(s,live).choices.map(c=>c.id))]];
 if(crisis&&r()<.5)return ['respondCrisis',[crisis.id,pick(r,E.crisisOptions(crisis).choices.map(c=>c.id))]];
 if(!s.executive.platform&&r()<.5){const goals=E.GOALS.map(g=>g.id).sort(()=>r()-.5).slice(0,3);return ['platform',[goals,pick(r,E.BILLS.map(b=>b.id))]];}
 if(Sim.campaignSeason(s)&&r()<.4)return s.location!=='aircraft'?['setLocation',['aircraft']]:['campaignTrip',[pick(r,Sim.REGIONS.map(x=>x.id)),pick(r,Object.keys(Sim.ISSUES)),pick(r,Sim.PEOPLE.map(p=>p.id))]];
 if(s.location!=='oval'&&r()<.5)return ['setLocation',['oval']];
 const P=Sim.P.map(p=>p.id),people=Sim.PEOPLE.map(p=>p.id),kind=pick(r,['changePolicy','changePolicy','changePolicy','meeting','meeting','campaignTrip','platform','propose','amend','amend','lobby','vote','regionalVisit','teamAction','respondCrisis','startEncounter','answerEncounter','answerEncounter','setLocation']);
 switch(kind){
  case 'changePolicy':return [kind,[pick(r,P),Math.floor(r()*5)]];
  case 'meeting':return [kind,[pick(r,people),pick(r,['promise','compromise','listen','reassure','extend','withdraw'])]];
  case 'campaignTrip':return [kind,[pick(r,Sim.REGIONS.map(x=>x.id)),pick(r,Object.keys(Sim.ISSUES)),pick(r,people)]];
  case 'platform':{const goals=E.GOALS.map(g=>g.id).sort(()=>r()-.5).slice(0,r()<.2?2:3);return [kind,[goals,pick(r,E.BILLS.map(b=>b.id))]];}
  case 'propose':return [kind,[pick(r,E.BILLS.map(b=>b.id))]];
  case 'amend':return [kind,[pick(r,E.AMENDMENTS.map(x=>x.id))]];
  case 'lobby':return [kind,[pick(r,E.BLOCS.map(x=>x.id))]];
  case 'vote':return [kind,[]];
  case 'regionalVisit':return [kind,[pick(r,E.REGIONS.map(x=>x.id))]];
  case 'teamAction':return [kind,[pick(r,people),pick(r,['support','train','replace'])]];
  case 'respondCrisis':return [kind,[pick(r,['storm','contracts']),pick(r,['prepare','watch','full','target','publish','defend'])]];
  case 'startEncounter':return [kind,[pick(r,['press','debate'])]];
  case 'answerEncounter':return [kind,[pick(r,['record','own','attack','specific','listen','timeline','victory','transparent','deflect'])]];
  case 'setLocation':return [kind,[pick(r,['oval','aircraft'])]];
 }
}
function play(start,botSeed,label){
 const r=bot(botSeed),s=start,steps=[];
 while(!s.ended){
  const n=Math.floor(r()*7);
  for(let i=0;i<n;i++){const [action,args]=botAction(r,s);steps.push({action,args,result:clone(ACTIONS[action](s,args))});}
  const choice=r()<.1?7:Math.floor(r()*3);steps.push({action:'choose',args:[choice],result:ACTIONS.choose(s,[choice])});
  if(!s.agendaChoice&&s.agendaChoice!==0)steps.push({action:'choose',args:[0],result:ACTIONS.choose(s,[0])});
  steps.push({action:'advance',args:[],result:clone(Sim.advance(s)),state:clone(s),derived:derived(s)});
 }
 return {label,steps};
}
const scenarios=[];
for(let i=0;i<24;i++){const seed=1000+i*7919,s=Sim.fresh(seed);scenarios.push({start:{fresh:seed},initial:clone(s),initialDerived:derived(s),...play(s,50+i,`fresh ${seed}`)});}
// Extreme starting agendas push groups into unrest, extremism and attempts on the president. The patch is merged into a fresh state.
const patches=[{capital:20,levels:{fossil:4,carbon:0,renewables:0,surveillance:0,police:0},metrics:{climate:12,liberty:30}},{levels:{clinic:0,insurance:0,prevention:0,rent:0,housing:0,minimum:0},metrics:{health:20,housing:20,jobs:30}},{debt:160,unrest:{anger:{workers:80,climate:60,students:30}},levels:{police:4,surveillance:4}}];
const merge=(a,b)=>{for(const [k,v] of Object.entries(b)){if(v&&typeof v==='object'&&!Array.isArray(v)&&a[k]&&typeof a[k]==='object')merge(a[k],v);else a[k]=v;}return a;};
patches.forEach((patch,i)=>{for(const seed of [77+i,3000+i]){const s=merge(Sim.fresh(seed),clone(patch));scenarios.push({start:{fresh:seed,patch},initial:clone(s),initialDerived:derived(s),...play(s,700+seed,`extreme ${i} seed ${seed}`)});}});
// Saves from earlier versions exercise migration.
const mk=(version,quarter,strip)=>{const s=Sim.fresh(4242);s.version=version;s.quarter=quarter;for(const k of strip)delete s[k];return clone(s);};
const olds=[mk(5,3,['unrest']),mk(4,6,['unrest','deck']),mk(2,5,['unrest','deck','relationships','appointments','promises','goodwill','campaign','executive']),{month:7,trust:63,capital:5,fiscal:41,location:'aircraft',history:[{month:0,event:'Old choice',choice:'Acted'}]}];
olds.forEach((old,i)=>{const s=Sim.migrate(clone(old));scenarios.push({start:{migrate:old},initial:clone(s),initialDerived:derived(s),...play(s,900+i,`migrate ${i}`)});});
const out=process.argv[2]||path.join(__dirname,'trace.json');
fs.writeFileSync(out,JSON.stringify({scenarios}));
const actions=scenarios.reduce((n,x)=>n+x.steps.length,0),ok=scenarios.reduce((n,x)=>n+x.steps.filter(s=>s.result===true||s.result?.ok).length,0);
console.log(`Trace written: ${scenarios.length} terms, ${actions} actions (${ok} succeeded), ${(fs.statSync(out).size/1e6).toFixed(1)} MB`);
