/* Four Years: original, deterministic policy simulation. No browser or 3D dependencies.
   Content (policies, voters, people, events, situations) lives in data/*.json and is loaded as FourYearsData. */
const FourYearsSim=(()=>{
 const bound=(n,a=0,b=100)=>Math.max(a,Math.min(b,n));
 if(typeof FourYearsData!=='object'||!FourYearsData)throw new Error('Four Years game data is missing: load data/*.json as FourYearsData before simulation-core.js.');
 const D=FourYearsData,METRICS=D.metrics.labels,BASE=D.metrics.base;
 // Each policy is a five-position lever. Effects are per quarter, after implementation catches up.
 const P=D.policies.map(p=>({...p,lag:p.lag??(p.cost>=8?3:p.cost>=5?2:1)}));
 const GROUPS=D.groups,PEOPLE=D.people,REGIONS=D.campaign.regions,ISSUES=D.campaign.issues,EVENTS=D.events,SITUATIONS=D.situations;
 validate();
 // Hand-edited data fails loudly with every problem listed, instead of producing NaN deep inside a quarter.
 function validate(){const errors=[],metric=k=>k in METRICS,metricOrDebt=k=>metric(k)||k==='debt',policyIds=new Set(P.map(p=>p.id)),groupIds=new Set(GROUPS.map(g=>g.id)),names=new Set(SITUATIONS.map(x=>x.name)),eventIds=new Set(),fixed=new Set();
  const keys=(where,obj,ok=metric)=>{for(const k of Object.keys(obj||{}))if(!ok(k))errors.push(`${where}: unknown key "${k}"`);};
  const ids=(where,list,known)=>{for(const id of list||[])if(!known.has(id))errors.push(`${where}: unknown id "${id}"`);};
  for(const p of P)keys(`policy ${p.id}`,p.effects);
  if(GROUPS.reduce((n,g)=>n+g.share,0)!==100)errors.push('voter group shares must add up to 100');
  for(const g of GROUPS){keys(`group ${g.id} priorities`,g.priorities);ids(`group ${g.id}`,[...g.fav,...g.opp],policyIds);}
  for(const p of PEOPLE){ids(`person ${p.id} policies`,p.policies,policyIds);ids(`person ${p.id} group`,[p.group],groupIds);}
  for(const r of REGIONS){ids(`region ${r.id} groups`,r.groups,groupIds);keys(`region ${r.id} issues`,Object.fromEntries(r.issues.map(i=>[i,1])),k=>k in ISSUES);}
  for(const x of SITUATIONS){if(!metricOrDebt(x.metric))errors.push(`situation ${x.name}: unknown metric "${x.metric}"`);
   if((x.below===undefined)===(x.above===undefined))errors.push(`situation ${x.name}: set exactly one of "below" or "above"`);
   else if(typeof x.clearsAt!=='number'||(x.below!==undefined?x.clearsAt<x.below:x.clearsAt>x.above))errors.push(`situation ${x.name}: clearsAt must sit on the recovery side of the trigger`);
   keys(`situation ${x.name} effects`,x.effects);keys(`situation ${x.name} groups`,x.groups,k=>groupIds.has(k));}
  for(const e of EVENTS){if(eventIds.has(e.id))errors.push(`event ${e.id}: duplicate id`);eventIds.add(e.id);
   if(e.fixed!==undefined){if(!Number.isInteger(e.fixed)||e.fixed<0||e.fixed>15||fixed.has(e.fixed))errors.push(`event ${e.id}: fixed must be a unique quarter from 0 to 15`);fixed.add(e.fixed);}
   const w=e.when||{};keys(`event ${e.id} when.below`,w.below,metricOrDebt);keys(`event ${e.id} when.above`,w.above,metricOrDebt);
   for(const name of [...(w.situations||[]),...(w.absent||[])])if(!names.has(name))errors.push(`event ${e.id}: unknown situation "${name}"`);
   if(!e.choices?.length)errors.push(`event ${e.id}: needs at least one choice`);
   for(const c of e.choices||[]){keys(`event ${e.id} choice "${c.label}"`,c.effects);if(typeof c.cost!=='number')errors.push(`event ${e.id} choice "${c.label}": cost must be a number`);}}
  if(errors.length)throw new Error('Four Years data errors:\n'+errors.join('\n'));}
 function situation(name){return SITUATIONS.find(x=>x.name===name);}
 function activeSituations(s){return (s.situations||[]).map(situation).filter(Boolean);}
 function metricValue(s,key){return key==='debt'?s.debt:s.metrics[key];}
 // A situation starts past its trigger and only clears once the condition recovers past clearsAt, so it cannot flicker.
 function updateSituations(s){const before=s.situations||[];return SITUATIONS.filter(x=>{const v=metricValue(s,x.metric),on=before.includes(x.name);return x.below!==undefined?v<(on?x.clearsAt:x.below):v>(on?x.clearsAt:x.above);}).map(x=>x.name);}
 // Event deck: fixed events keep their quarter; the rest are drawn by weight from events whose conditions hold, reproducibly from the save's seed.
 function eventEligible(s,e){const w=e.when||{},q=s.quarter;if((w.from!==undefined&&q<w.from)||(w.until!==undefined&&q>w.until))return false;
  if(Object.entries(w.below||{}).some(([k,v])=>!(metricValue(s,k)<v))||Object.entries(w.above||{}).some(([k,v])=>!(metricValue(s,k)>v)))return false;
  return (w.situations||[]).every(n=>s.situations.includes(n))&&!(w.absent||[]).some(n=>s.situations.includes(n));}
 function roll(seed,quarter){let t=(seed^Math.imul(quarter+1,0x9e3779b1))>>>0;t=Math.imul(t^t>>>15,t|1);t^=t+Math.imul(t^t>>>7,t|61);return ((t^t>>>14)>>>0)/4294967296;}
 function drawEvent(s){const used=new Set(s.deck.used),open=EVENTS.filter(e=>!used.has(e.id)),flexible=open.filter(e=>e.fixed===undefined);
  let event=open.find(e=>e.fixed===s.quarter);
  if(!event){const eligible=flexible.filter(e=>eventEligible(s,e)),pool=eligible.length?eligible:flexible.length?flexible:EVENTS.filter(e=>e.fixed===undefined),total=pool.reduce((n,e)=>n+(e.weight??1),0);
   let r=roll(s.seed,s.quarter)*total;event=pool.find(e=>(r-=(e.weight??1))<0)||pool[pool.length-1];}
  s.deck.current=event.id;if(!used.has(event.id))s.deck.used.push(event.id);return event;}
 function currentEvent(s){return EVENTS.find(e=>e.id===s.deck?.current)||null;}
 // Saves from before the deck continue with the event they were already shown.
 function legacyDeck(s){return {used:EVENTS.filter(e=>e.legacyQuarter!==undefined&&e.legacyQuarter<=s.quarter).map(e=>e.id),current:s.ended?null:EVENTS.find(e=>e.legacyQuarter===s.quarter)?.id??null};}
 function newSeed(){return Math.floor(Math.random()*2147483647);}
 function politicalDefaults(){return {relationships:Object.fromEntries(PEOPLE.map(p=>[p.id,50])),appointments:[],promises:[],goodwill:{},campaign:{boosts:{},trips:[]}};}
 function upgrade(s){const defaults=politicalDefaults();for(const [k,v] of Object.entries(defaults))if(s[k]===undefined)s[k]=v;if(!Number.isInteger(s.seed))s.seed=6137;if(!s.situations)s.situations=[];if(!s.deck)s.deck=legacyDeck(s);s.version=5;return ExecutiveSim.initialize(s);}
 function fresh(seed=6137){const levels=Object.fromEntries(P.map(p=>[p.id,2]));const s=ExecutiveSim.initialize({version:5,quarter:0,location:'oval',levels,implemented:{...levels},metrics:{...BASE},capital:12,debt:72,budget:null,agendaChoice:null,history:[],effects:[],situations:[],polls:[],midterm:null,ended:false,legacy:null,seed,deck:{used:[],current:null},...politicalDefaults()});drawEvent(s);return s;}
 function migrate(old){if(!old||typeof old!=='object')return fresh();if([2,3,4,5].includes(old.version)){const s=upgrade(old);if(!s.ended&&!currentEvent(s)){s.agendaChoice=null;drawEvent(s);}return s;}const s=fresh();s.location=old.location==='aircraft'?'aircraft':'oval';s.quarter=bound(Math.floor((old.month||0)/3),0,15);s.metrics.trust=bound(Number(old.trust)||53);s.metrics.growth=bound(Number(old.fiscal)||50);s.capital=bound(Math.round((Number(old.capital)||7)*1.5),0,20);s.deck=legacyDeck(s);s.legacy={months:old.month||0,history:old.history||[],trust:old.trust,fiscal:old.fiscal};s.history.push({quarter:s.quarter,type:'legacy',title:'First-year record preserved',detail:`${s.legacy.months} monthly briefings from the earlier prototype are archived below.`});return s;}
 function people(s){return PEOPLE.map(p=>ExecutiveSim.person(s,p));}
 function availableSlots(s){return Math.max(0,2-s.appointments.filter(a=>a.quarter===s.quarter).length);}
 function campaignSeason(s){return !s.ended&&[6,7,14,15].includes(s.quarter);}
 function request(s,id){const person=people(s).find(p=>p.id===id);if(!person)return null;const active=s.promises.find(p=>p.person===id&&p.status==='open');const candidates=person.policies.map((_,i)=>person.policies[(i+s.quarter)%person.policies.length]);const policyId=active?.policy||candidates.find(id=>s.levels[id]<4)||candidates[0];return {person,policy:policy(policyId),active,target:Math.min(4,s.levels[policyId]+1),done:s.appointments.some(a=>a.quarter===s.quarter&&a.person===id)};}
 function log(s,type,title,detail){s.history.push({quarter:s.quarter,type,title,detail});}
 function meeting(s,id,response){const r=request(s,id);if(!r||s.executive.cabinet[id].status==='resigned'||r.done||s.ended||s.agendaChoice!==null||!availableSlots(s)||s.location!=='oval')return {ok:false,reason:'Meetings need an open appointment slot in the Oval Office before filing the quarterly decision.'};
  const allowed=r.active?['reassure','extend','withdraw']:['promise','compromise','listen'];if(!allowed.includes(response))return {ok:false,reason:'Choose a response offered in this meeting.'};
  if(response==='extend'&&(r.active.extended||r.active.due>=16))return {ok:false,reason:'This deadline cannot be extended again.'};
  if(response==='reassure'&&s.capital<1)return {ok:false,reason:'Reassurance costs 1 political capital.'};
  if(response==='promise'&&s.levels[r.policy.id]>=4)return {ok:false,reason:'This program is already at full strength. Choose a listening meeting.'};
  if(response==='compromise'&&s.levels[r.policy.id]>=4)return {ok:false,reason:'This program is already at full strength. Choose a listening meeting.'};
  s.appointments.push({quarter:s.quarter,person:id,type:'meeting'});let detail='';
  if(response==='promise'||response==='compromise'){const support=response==='promise'?3:1,due=Math.min(16,s.quarter+(response==='promise'?2:3));s.promises.push({id:`${s.quarter}-${id}`,person:id,policy:r.policy.id,target:r.target,due,status:'open',extended:false});s.capital=bound(s.capital+support,0,20);s.relationships[id]=bound(s.relationships[id]+(response==='promise'?4:2));detail=`${r.policy.name} at ${r.target}/4 by the end of term quarter ${due}/16. ${response==='compromise'?'A longer deadline earns narrower support.':'A firm pledge earns immediate support.'} Political capital +${support}.`;}
  if(response==='listen'){s.relationships[id]=bound(s.relationships[id]+1);detail='You listened without making a commitment. Relationship +1; no political support pledged.';}
  if(response==='reassure'){s.capital--;s.relationships[id]=bound(s.relationships[id]+3);detail='You invested political attention in the relationship. Capital −1; relationship +3. The original deadline still stands.';}
  if(response==='extend'){r.active.due=Math.min(16,r.active.due+1);r.active.extended=true;s.relationships[id]=bound(s.relationships[id]-3);detail=`A reluctant extension to quarter ${r.active.due}. Relationship −3.`;}
  if(response==='withdraw'){resolvePromise(s,r.active,false);detail='You withdrew the promise. Trust and the relationship take the same hit as a missed deadline.';}
  log(s,'meeting',`Meeting with ${r.person.name}`,detail);return {ok:true,detail};
 }
 function resolvePromise(s,p,kept){const person=people(s).find(x=>x.id===p.person);p.status=kept?'kept':'broken';p.resolved=s.quarter;s.relationships[p.person]=bound(s.relationships[p.person]+(kept?8:-12));s.goodwill[person.group]=bound((s.goodwill[person.group]||0)+(kept?2:-3),-12,12);s.metrics.trust=bound(s.metrics.trust+(kept?1:-3));if(kept)s.capital=bound(s.capital+2,0,20);const title=`${kept?'Promise kept':'Promise broken'}: ${policy(p.policy).name}`;log(s,'promise',title,`${person.name} ${kept?'backs your administration':'questions your word'}. Relationship ${kept?'+8':'−12'}; ${GROUPS.find(g=>g.id===person.group).name} goodwill ${kept?'+2':'−3'}.`);return `${title}. ${person.name} ${kept?'celebrates delivery':'demands an explanation'}.`;}
 function tripPreview(s,regionId,issueId,personId){const region=REGIONS.find(r=>r.id===regionId),issue=ISSUES[issueId],person=people(s).find(p=>p.id===personId);if(!region||!issue||!person)return null;const relevant=region.issues.includes(issueId),credible=s.metrics[issue.metric]>=55,expert=person.expertise.includes(issueId),trusted=s.relationships[personId]>=60;const strength=Math.max(1,(relevant?2:1)+(credible?1:0)+(expert?1:0)+(trusted?1:0)-Math.min(2,s.promises.filter(p=>p.status==='broken'&&p.person===personId).length));return {region,issue,person,strength,relevant,credible,expert,trusted};}
 function campaignTrip(s,regionId,issueId,personId){const t=tripPreview(s,regionId,issueId,personId);if(!t||s.executive.cabinet[personId]?.status==='resigned')return {ok:false,reason:'Choose a region, a speech and a traveling adviser.'};if(!campaignSeason(s)||s.agendaChoice!==null)return {ok:false,reason:'Campaign flights open in the two quarters before each election, before filing the quarterly decision.'};if(s.location!=='aircraft')return {ok:false,reason:'Board Air Force One to hold your campaign briefing.'};if(!availableSlots(s)||s.campaign.trips.some(x=>x.quarter===s.quarter))return {ok:false,reason:'You need an open appointment slot. One campaign trip is allowed each quarter.'};if(s.capital<2)return {ok:false,reason:'A campaign trip requires 2 political capital.'};s.capital-=2;s.debt=bound(s.debt+2,0,250);s.executive.regions[regionId].goodwill=bound(s.executive.regions[regionId].goodwill+t.strength*.5,-15,15);s.appointments.push({quarter:s.quarter,type:'trip'});s.campaign.trips.push({quarter:s.quarter,region:regionId,issue:issueId,person:personId,strength:t.strength});for(const group of t.region.groups)s.campaign.boosts[group]=bound((s.campaign.boosts[group]||0)+t.strength,0,8);const detail=`${t.person.name} joins you in ${t.region.name}. Speech: ${t.issue.name}. Local voter support +${t.strength}; capital −2; debt +2. Campaign enthusiasm fades by 25% each quarter.`;log(s,'campaign',`On the road: ${t.region.name}`,detail);return {ok:true,detail};}
 function policy(id){return P.find(p=>p.id===id)}
 function budget(s){let revenue=87,spending=85;for(const p of P){const contribution=(s.levels[p.id]-2)*p.cost;if(p.cost<0)revenue-=contribution;else spending+=contribution;}revenue+=(s.metrics.growth-50)*.2+activeSituations(s).reduce((n,x)=>n+(x.revenue||0),0);spending+=s.debt*.045+ExecutiveSim.spending(s);return {revenue:Math.round(revenue),spending:Math.round(spending),balance:Math.round(revenue-spending),interest:Math.round(s.debt*.045)};}
 function groups(s){return GROUPS.map(g=>{let weighted=0,total=0;for(const [key,w] of Object.entries(g.priorities)){weighted+=(s.metrics[key]-50)*w;total+=w;}let stance=0;for(const id of g.fav)stance+=(s.levels[id]-2)*1.8;for(const id of g.opp)stance-=(s.levels[id]-2)*1.8;const approval=bound(Math.round(51+weighted/Math.max(1,total)*.85+stance+(s.goodwill?.[g.id]||0)+(s.campaign?.boosts[g.id]||0)+activeSituations(s).reduce((n,x)=>n+(x.groups?.[g.id]||0),0)+ExecutiveSim.modifier(s,g.id)),5,95);return {...g,approval,turnout:bound(Math.round(58+Math.abs(approval-50)*.3+(g.id==='retirees'?10:0)),40,85)};});}
 function poll(s){const gs=groups(s),total=gs.reduce((a,g)=>a+g.share*g.turnout,0);return Math.round(gs.reduce((a,g)=>a+g.share*g.turnout*g.approval,0)/total);}
 function changePolicy(s,id,level){const p=policy(id);if(!p||s.ended||s.agendaChoice!==null||level<0||level>4||!Number.isInteger(level))return {ok:false,reason:'This policy cannot be changed now.'};if(level===4&&ExecutiveSim.BILLS.some(b=>b.policy===id)&&!s.executive.bills.some(b=>b.status==='passed'&&ExecutiveSim.BILLS.find(d=>d.id===b.id).policy===id))return {ok:false,reason:'Full-strength funding needs an act of Congress. Open Congress to introduce the bill.'};const previous=s.levels[id],steps=Math.abs(level-previous);if(!steps)return {ok:false,reason:'That setting is already in force.'};const cost=steps*(2+(p.lag>1?1:0));if(s.capital<cost)return {ok:false,reason:`Requires ${cost} political capital.`};s.capital-=cost;s.levels[id]=level;s.history.push({quarter:s.quarter,type:'policy',title:p.name,detail:`Set to ${level}/4 · ${cost} political capital`});return {ok:true,cost};}
 function choose(s,index){if(s.ended||s.agendaChoice!==null)return false;const event=currentEvent(s),choice=event?.choices[index];if(!choice)return false;s.agendaChoice=index;s.effects.push({due:s.quarter+1,delta:choice.effects,title:`${event.title}: ${choice.label}`});s.debt=bound(s.debt+choice.cost*.45,0,250);s.history.push({quarter:s.quarter,type:'agenda',event:event.id,title:event.title,detail:`${choice.label} · ${choice.cost<0?`Saves ${-choice.cost}`:`Cost ${choice.cost}`} budget units; effects arrive next quarter.`});return true;}
 function advance(s){if(s.ended||s.agendaChoice===null)return false;upgrade(s);const report={quarter:s.quarter+1,changes:[],situations:[],budget:null,election:null};s.quarter++;
  for(const promise of s.promises.filter(p=>p.status==='open')){if(s.levels[promise.policy]>=promise.target)report.changes.push(resolvePromise(s,promise,true));else if(s.quarter>=promise.due)report.changes.push(resolvePromise(s,promise,false));}
  for(const group of Object.keys(s.campaign.boosts))s.campaign.boosts[group]*=.75;
  for(const p of P){const current=s.implemented[p.id],target=s.levels[p.id];s.implemented[p.id]=current+(target-current)*Math.min(1,ExecutiveSim.staffFactor(s,p.id)/p.lag);}
  const incoming=Object.fromEntries(Object.keys(BASE).map(k=>[k,0]));
  for(const p of P)for(const [key,value] of Object.entries(p.effects))incoming[key]+=(s.implemented[p.id]-2)*value;
  for(const effect of s.effects.filter(e=>e.due===s.quarter)){for(const [key,value] of Object.entries(effect.delta))incoming[key]+=value;report.changes.push(effect.title);s.history.push({quarter:s.quarter,type:'effect',title:effect.title,detail:Object.entries(effect.delta).map(([k,v])=>`${METRICS[k]} ${v>0?'+':''}${v}`).join(' · ')});}
  s.effects=s.effects.filter(e=>e.due>s.quarter);
  // Active situations are part of the web: they push on conditions until they clear.
  for(const x of activeSituations(s))for(const [key,value] of Object.entries(x.effects||{}))incoming[key]+=value;
  // Conditions feed back into each other, while inertia stops a lever from solving a crisis instantly.
  incoming.growth+=(s.metrics.jobs-50)*.018-Math.max(0,s.debt-100)*.025;
  incoming.jobs+=(s.metrics.growth-50)*.025;incoming.housing+=(s.metrics.jobs-50)*.025;incoming.health+=(s.metrics.housing-50)*.015;incoming.energy+=(s.metrics.climate-50)*.018;
  incoming.trust+=(s.metrics.jobs-50)*.03+(s.metrics.prices-50)*.025-Math.max(0,s.debt-110)*.02-Math.max(0,-(s.budget?.balance||0)-8)*.03;
  const previous={...s.metrics};for(const key of Object.keys(BASE))s.metrics[key]=bound(Math.round((s.metrics[key]+(BASE[key]+incoming[key]-s.metrics[key])*.36)*10)/10,0,100);
  report.changes.push(...Object.keys(BASE).filter(k=>Math.abs(s.metrics[k]-previous[k])>=1.4).map(k=>`${METRICS[k]} ${s.metrics[k]>previous[k]?'rose':'fell'} to ${Math.round(s.metrics[k])}`));
  const b=budget(s);s.budget=b;s.debt=bound(Math.round((s.debt-b.balance*.3)*10)/10,0,250);report.budget=b;
  const before=s.situations;s.situations=updateSituations(s);report.situations=[...s.situations];
  for(const name of s.situations)if(!before.includes(name))report.changes.push(`New situation: ${name}`);
  for(const name of before)if(!s.situations.includes(name)&&situation(name))report.changes.push(`Situation eased: ${name}`);
  ExecutiveSim.quarter(s,report);
  const currentPoll=poll(s);s.polls.push({quarter:s.quarter,approval:currentPoll});s.capital=bound(s.capital+(currentPoll>=55?4:currentPoll>=42?3:2)+activeSituations(s).reduce((n,x)=>n+(x.capital||0),0),0,20);
  if(s.quarter===8){const regional=ExecutiveSim.election(s,currentPoll);s.midterm={vote:currentPoll,majority:regional.won,regional};s.executive.mandate=regional.won?5:-5;if(!s.midterm.majority)s.capital=bound(s.capital-3,0,20);report.election={type:'midterm',...s.midterm};}
  if(s.quarter===16){s.ended=true;report.election={type:'general',vote:currentPoll,...ExecutiveSim.election(s,currentPoll)};s.executive.finalElection=report.election;}
  s.agendaChoice=null;if(!s.ended)drawEvent(s);s.history.push({quarter:s.quarter,type:'report',title:`Quarter ${s.quarter} report`,detail:`Polling ${currentPoll}% · budget ${b.balance>=0?'+':''}${b.balance} · debt ${Math.round(s.debt)}`});return report;}
 return {executive:ExecutiveSim,people,METRICS,BASE,P,GROUPS,EVENTS,SITUATIONS,PEOPLE,REGIONS,ISSUES,situation,updateSituations,currentEvent,eventEligible,newSeed,fresh,migrate,policy,budget,groups,poll,changePolicy,choose,advance,bound,availableSlots,campaignSeason,request,meeting,tripPreview,campaignTrip};
})();
