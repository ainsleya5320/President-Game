/* Four Years: original, deterministic policy simulation. No browser or 3D dependencies. */
const FourYearsSim=(()=>{
 const bound=(n,a=0,b=100)=>Math.max(a,Math.min(b,n));
 const METRICS={growth:'Economic growth',jobs:'Employment',prices:'Price stability',health:'Public health',schools:'Education',housing:'Housing access',safety:'Public safety',climate:'Climate resilience',energy:'Energy reliability',equality:'Economic mobility',liberty:'Civil liberties',trust:'Institutional trust'};
 const BASE={growth:51,jobs:52,prices:54,health:51,schools:50,housing:45,safety:57,climate:44,energy:56,equality:45,liberty:60,trust:53};
 // Each policy is a five-position lever. Effects are per quarter, after implementation catches up.
 const P=[
 ['tax','Progressive tax','Treasury',-7,{equality:2.8,housing:1,growth:-1.2,trust:.6}],
 ['corporate','Business tax','Treasury',-6,{equality:1.3,growth:-1.6,jobs:-.7}],
 ['sales','Consumption tax','Treasury',-5,{prices:-1.1,equality:-1.8,growth:-.4}],
 ['stimulus','Small business grants','Economy',7,{growth:2.2,jobs:2.3,prices:-.7}],
 ['roads','Transport investment','Economy',8,{growth:1.5,jobs:1.2,safety:1,climate:-.7}],
 ['trade','Open trade','Economy',-2,{growth:1.6,prices:1.3,jobs:-.8}],
 ['minimum','Wage floor','Economy',1,{equality:2.2,jobs:-.5,prices:-.8}],
 ['clinic','Community clinics','Health',8,{health:2.7,equality:1.1,trust:.7}],
 ['insurance','Health coverage','Health',11,{health:2.4,equality:2,trust:1}],
 ['prevention','Prevention programs','Health',5,{health:1.8,schools:.5}],
 ['teacher','Teacher support','Education',7,{schools:2.6,jobs:.4,equality:.6}],
 ['college','Skills and apprenticeships','Education',6,{schools:1.5,jobs:1.7,equality:.8}],
 ['housing','Affordable construction','Housing',9,{housing:3.1,jobs:1.2,prices:-.3}],
 ['zoning','Local zoning incentives','Housing',3,{housing:2.1,liberty:-.5,trust:-.3}],
 ['rent','Rental assistance','Housing',7,{housing:1.8,equality:1.4,prices:-.6}],
 ['police','Community policing','Justice',6,{safety:2.3,liberty:-.7,trust:.5}],
 ['rehab','Rehabilitation services','Justice',5,{safety:1.4,health:.8,equality:1}],
 ['surveillance','Security surveillance','Justice',5,{safety:2.3,liberty:-2.7,trust:-.8}],
 ['renewables','Clean power','Environment',9,{climate:2.7,energy:1,prices:-.6}],
 ['carbon','Carbon levy','Environment',-6,{climate:2.5,prices:-2,growth:-.7,equality:-.8}],
 ['grid','Grid modernization','Environment',8,{energy:2.8,climate:.7,jobs:.5}],
 ['fossil','Domestic fuel support','Environment',5,{energy:2.2,prices:1.1,climate:-2.8}],
 ['diplomacy','Diplomatic service','Foreign affairs',4,{trust:1.1,growth:.7,safety:.3}],
 ['transparency','Government transparency','Government',3,{trust:2.5,liberty:1.2}],
 ['elections','Election access','Government',4,{liberty:2,trust:1.4}],
 ['research','Public research','Government',6,{growth:1.3,health:.8,energy:.5}]
 ].map(([id,name,department,cost,effects])=>({id,name,department,cost,effects,lag:cost>=8?3:cost>=5?2:1}));
 const GROUPS=[
  {id:'workers',name:'Working families',share:19,priorities:{jobs:2,equality:1.5,prices:1.4,housing:1},fav:['minimum','clinic','housing'],opp:['sales']},
  {id:'business',name:'Business owners',share:12,priorities:{growth:2,prices:1.4,energy:1},fav:['trade','grid'],opp:['corporate','minimum']},
  {id:'retirees',name:'Retirees',share:15,priorities:{health:2.3,prices:1.5,safety:1},fav:['insurance','clinic'],opp:['sales']},
  {id:'students',name:'Students & young adults',share:11,priorities:{schools:1.6,housing:1.5,climate:1.2,liberty:1},fav:['college','rent'],opp:['surveillance']},
  {id:'suburban',name:'Suburban households',share:14,priorities:{schools:1.4,safety:1.3,prices:1.3,trust:1},fav:['teacher','roads'],opp:['carbon']},
  {id:'rural',name:'Rural communities',share:10,priorities:{energy:1.6,jobs:1.5,health:1.2},fav:['roads','fossil'],opp:['carbon']},
  {id:'climate',name:'Climate advocates',share:7,priorities:{climate:3,liberty:1,trust:.7},fav:['renewables','carbon'],opp:['fossil']},
  {id:'civil',name:'Civil-liberties voters',share:5,priorities:{liberty:3,trust:1.5},fav:['transparency','elections'],opp:['surveillance']},
  {id:'health',name:'Health-care voters',share:4,priorities:{health:3,equality:1},fav:['clinic','insurance'],opp:[]},
  {id:'fiscal',name:'Fiscal conservatives',share:3,priorities:{growth:1.3,prices:1.3,trust:1},fav:['trade'],opp:['stimulus','insurance','rent']}
 ];
 const AGENDA=[
  {title:'The grid under pressure',body:'A heat wave has strained regional power lines. Governors are asking for immediate federal help.',choices:[['Release emergency funds',{energy:5,trust:2},7],['Coordinate a limited response',{energy:2,trust:1},3],['Ask states to manage it',{energy:-3,trust:-2},0]]},
  {title:'Factory town layoffs',body:'A major manufacturer plans to close a plant. Workers want an answer before the next news cycle.',choices:[['Fund retraining',{jobs:2,schools:1},6],['Negotiate a bridge loan',{jobs:3,growth:1},9],['Let the market adjust',{jobs:-2,trust:-2},0]]},
  {title:'School building failures',body:'A national survey finds hundreds of unsafe classrooms. The request for repairs arrives with a large price tag.',choices:[['Launch a repair fund',{schools:4,trust:1},9],['Offer matching grants',{schools:2},5],['Defer to districts',{schools:-2,trust:-1},0]]},
  {title:'Port congestion',body:'Imports are waiting offshore. Stores warn of higher prices if the bottleneck continues.',choices:[['Expand port capacity',{growth:2,prices:2,climate:-1},7],['Temporarily streamline inspection',{prices:2,safety:-1},2],['Hold current standards',{prices:-2},0]]},
  {title:'A health system warning',body:'Clinics report understaffing. Medical leaders say waiting will cost more later.',choices:[['Recruit and train clinicians',{health:3,jobs:1},8],['Target the worst regions',{health:2},4],['Request a private-sector plan',{health:-2,trust:-1},0]]},
  {title:'Housing cost surge',body:'Rents are climbing faster than wages in several cities.',choices:[['Build mixed-income housing',{housing:4,jobs:1},9],['Temporary rent grants',{housing:2,equality:1},6],['Leave action to cities',{housing:-2,trust:-1},0]]},
  {title:'Storm season',body:'A severe storm has damaged roads and substations. Emergency crews are working at capacity.',choices:[['Full federal mobilization',{energy:3,safety:2,trust:2},9],['Target the worst-hit counties',{energy:2,safety:1},5],['Rely on local reserves',{energy:-3,safety:-2},0]]},
  {title:'A watchdog report',body:'Auditors find waste in a major contract. The cabinet wants time before releasing the report.',choices:[['Publish and reform',{trust:4,liberty:1},4],['Commission another review',{trust:-1},1],['Defend the program',{trust:-4},0]]},
  {title:'The midterm aftermath',body:'The new legislature expects a fiscal proposal within weeks.',choices:[['Offer a bipartisan package',{trust:2,growth:1},5],['Push the full agenda',{trust:-1,equality:2},7],['Pause major bills',{trust:-2},0]]},
  {title:'Energy price spike',body:'Fuel costs are feeding into grocery and commuting bills.',choices:[['Subsidize essential energy',{prices:3,energy:2,climate:-1},8],['Accelerate grid storage',{energy:2,climate:2},7],['Wait for prices to settle',{prices:-3,trust:-1},0]]},
  {title:'Public safety debate',body:'Mayors demand help with a rise in violent crime. Civil-rights groups warn against a broad crackdown.',choices:[['Fund local prevention',{safety:2,health:1},6],['Expand federal enforcement',{safety:3,liberty:-2},5],['Keep existing strategy',{safety:-2},0]]},
  {title:'Research breakthrough',body:'A new medical technique could be tested nationally, but the evidence is still preliminary.',choices:[['Fund clinical trials',{health:2,growth:1},7],['Run a small pilot',{health:1},3],['Decline federal support',{trust:-1},0]]},
  {title:'A trade dispute',body:'A partner country threatens tariffs on domestic exports.',choices:[['Negotiate privately',{growth:1,trust:1},3],['Retaliate with tariffs',{jobs:1,prices:-2,growth:-1},2],['Accept their terms',{growth:-2,trust:-2},0]]},
  {title:'Cities ask for transit',body:'Urban leaders propose a national transit initiative. Rural representatives say their roads need attention too.',choices:[['Fund transit and rural roads',{growth:2,climate:2,trust:1},9],['Fund targeted transit',{climate:2,growth:1},5],['Delay the decision',{trust:-2},0]]},
  {title:'A difficult budget',body:'Interest payments are growing. The budget director says every new dollar needs a source.',choices:[['Raise revenue',{equality:2,growth:-1,prices:-1},3],['Trim programs',{growth:-1,equality:-2,trust:-1},0],['Borrow for another year',{growth:1,trust:-1},10]]},
  {title:'The final address',body:'The country will soon judge the full term. What will you emphasize?',choices:[['Explain the hard tradeoffs',{trust:2},1],['Promise a new agenda',{trust:1,growth:1},3],['Focus on achievements',{trust:1},0]]}
 ];
 const PEOPLE=[
  {id:'chen',name:'Maya Chen',role:'Housing secretary',initials:'MC',group:'workers',policies:['housing','rent','clinic','teacher'],expertise:['housing','health'],quote:'People need to see progress where they live.'},
  {id:'ruiz',name:'Elena Ruiz',role:'Governor & regional ally',initials:'ER',group:'rural',policies:['roads','grid','college','fossil'],expertise:['jobs','energy'],quote:'Give my communities a reason to stand with you.'},
  {id:'brooks',name:'Daniel Brooks',role:'Treasury secretary',initials:'DB',group:'fiscal',policies:['tax','corporate','sales','trade'],expertise:['jobs','health'],quote:'A promise needs a way to pay for it.'},
  {id:'bell',name:'Samira Bell',role:'Congressional leader',initials:'SB',group:'suburban',policies:['teacher','transparency','police','elections'],expertise:['housing','energy'],quote:'I can bring votes. I need something to take back to them.'}
 ];
 const REGIONS=[
  {id:'lakes',name:'Great Lakes',groups:['workers','business'],issues:['jobs','housing'],description:'Factory towns and growing cities want secure jobs and affordable homes.'},
  {id:'coast',name:'Coastal cities',groups:['students','climate','civil'],issues:['housing','energy'],description:'Young renters want housing and a credible clean-energy plan.'},
  {id:'plains',name:'Plains & small towns',groups:['rural','suburban'],issues:['energy','jobs'],description:'Energy prices, infrastructure and local employers lead the conversation.'},
  {id:'sunbelt',name:'Sunbelt communities',groups:['retirees','health','fiscal'],issues:['health','housing'],description:'Health care and the cost of living dominate these fast-growing communities.'}
 ];
 const ISSUES={housing:{name:'Homes people can afford',metric:'housing'},jobs:{name:'Good jobs close to home',metric:'jobs'},health:{name:'Reliable health care',metric:'health'},energy:{name:'Affordable, reliable energy',metric:'energy'}};
 function politicalDefaults(){return {relationships:Object.fromEntries(PEOPLE.map(p=>[p.id,50])),appointments:[],promises:[],goodwill:{},campaign:{boosts:{},trips:[]}};}
 function upgrade(s){const defaults=politicalDefaults();for(const [k,v] of Object.entries(defaults))if(s[k]===undefined)s[k]=v;s.version=3;return s;}
 function fresh(){const levels=Object.fromEntries(P.map(p=>[p.id,2]));return {version:3,quarter:0,location:'oval',levels,implemented:{...levels},metrics:{...BASE},capital:12,debt:72,budget:null,agendaChoice:null,history:[],effects:[],situations:[],polls:[],midterm:null,ended:false,legacy:null,seed:6137,...politicalDefaults()};}
 function migrate(old){if(!old||typeof old!=='object')return fresh();if(old.version===2||old.version===3)return upgrade(old);const s=fresh();s.location=old.location==='aircraft'?'aircraft':'oval';s.quarter=bound(Math.floor((old.month||0)/3),0,15);s.metrics.trust=bound(Number(old.trust)||53);s.metrics.growth=bound(Number(old.fiscal)||50);s.capital=bound(Math.round((Number(old.capital)||7)*1.5),0,20);s.legacy={months:old.month||0,history:old.history||[],trust:old.trust,fiscal:old.fiscal};s.history.push({quarter:s.quarter,type:'legacy',title:'First-year record preserved',detail:`${s.legacy.months} monthly briefings from the earlier prototype are archived below.`});return s;}
 function availableSlots(s){return Math.max(0,2-s.appointments.filter(a=>a.quarter===s.quarter).length);}
 function campaignSeason(s){return !s.ended&&[6,7,14,15].includes(s.quarter);}
 function request(s,id){const person=PEOPLE.find(p=>p.id===id);if(!person)return null;const active=s.promises.find(p=>p.person===id&&p.status==='open');const candidates=person.policies.map((_,i)=>person.policies[(i+s.quarter)%person.policies.length]);const policyId=active?.policy||candidates.find(id=>s.levels[id]<4)||candidates[0];return {person,policy:policy(policyId),active,target:Math.min(4,s.levels[policyId]+1),done:s.appointments.some(a=>a.quarter===s.quarter&&a.person===id)};}
 function log(s,type,title,detail){s.history.push({quarter:s.quarter,type,title,detail});}
 function meeting(s,id,response){const r=request(s,id);if(!r||r.done||s.ended||s.agendaChoice!==null||!availableSlots(s)||s.location!=='oval')return {ok:false,reason:'Meetings need an open appointment slot in the Oval Office before filing the quarterly decision.'};
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
 function resolvePromise(s,p,kept){const person=PEOPLE.find(x=>x.id===p.person);p.status=kept?'kept':'broken';p.resolved=s.quarter;s.relationships[p.person]=bound(s.relationships[p.person]+(kept?8:-12));s.goodwill[person.group]=bound((s.goodwill[person.group]||0)+(kept?2:-3),-12,12);s.metrics.trust=bound(s.metrics.trust+(kept?1:-3));if(kept)s.capital=bound(s.capital+2,0,20);const title=`${kept?'Promise kept':'Promise broken'}: ${policy(p.policy).name}`;log(s,'promise',title,`${person.name} ${kept?'backs your administration':'questions your word'}. Relationship ${kept?'+8':'−12'}; ${GROUPS.find(g=>g.id===person.group).name} goodwill ${kept?'+2':'−3'}.`);return `${title}. ${person.name} ${kept?'celebrates delivery':'demands an explanation'}.`;}
 function tripPreview(s,regionId,issueId,personId){const region=REGIONS.find(r=>r.id===regionId),issue=ISSUES[issueId],person=PEOPLE.find(p=>p.id===personId);if(!region||!issue||!person)return null;const relevant=region.issues.includes(issueId),credible=s.metrics[issue.metric]>=55,expert=person.expertise.includes(issueId),trusted=s.relationships[personId]>=60;const strength=Math.max(1,(relevant?2:1)+(credible?1:0)+(expert?1:0)+(trusted?1:0)-Math.min(2,s.promises.filter(p=>p.status==='broken'&&p.person===personId).length));return {region,issue,person,strength,relevant,credible,expert,trusted};}
 function campaignTrip(s,regionId,issueId,personId){const t=tripPreview(s,regionId,issueId,personId);if(!t)return {ok:false,reason:'Choose a region, a speech and a traveling adviser.'};if(!campaignSeason(s)||s.agendaChoice!==null)return {ok:false,reason:'Campaign flights open in the two quarters before each election, before filing the quarterly decision.'};if(s.location!=='aircraft')return {ok:false,reason:'Board Air Force One to hold your campaign briefing.'};if(!availableSlots(s)||s.campaign.trips.some(x=>x.quarter===s.quarter))return {ok:false,reason:'You need an open appointment slot. One campaign trip is allowed each quarter.'};if(s.capital<2)return {ok:false,reason:'A campaign trip requires 2 political capital.'};s.capital-=2;s.debt=bound(s.debt+2,0,250);s.appointments.push({quarter:s.quarter,type:'trip'});s.campaign.trips.push({quarter:s.quarter,region:regionId,issue:issueId,person:personId,strength:t.strength});for(const group of t.region.groups)s.campaign.boosts[group]=bound((s.campaign.boosts[group]||0)+t.strength,0,8);const detail=`${t.person.name} joins you in ${t.region.name}. Speech: ${t.issue.name}. Local voter support +${t.strength}; capital −2; debt +2. Campaign enthusiasm fades by 25% each quarter.`;log(s,'campaign',`On the road: ${t.region.name}`,detail);return {ok:true,detail};}
 function policy(id){return P.find(p=>p.id===id)}
 function budget(s){let revenue=87,spending=85;for(const p of P){const contribution=(s.levels[p.id]-2)*p.cost;if(p.cost<0)revenue-=contribution;else spending+=contribution;}revenue+=(s.metrics.growth-50)*.2;spending+=s.debt*.045;return {revenue:Math.round(revenue),spending:Math.round(spending),balance:Math.round(revenue-spending),interest:Math.round(s.debt*.045)};}
 function groups(s){return GROUPS.map(g=>{let weighted=0,total=0;for(const [key,w] of Object.entries(g.priorities)){weighted+=(s.metrics[key]-50)*w;total+=w;}let stance=0;for(const id of g.fav)stance+=(s.levels[id]-2)*1.8;for(const id of g.opp)stance-=(s.levels[id]-2)*1.8;const approval=bound(Math.round(51+weighted/Math.max(1,total)*.85+stance+(s.goodwill?.[g.id]||0)+(s.campaign?.boosts[g.id]||0)),5,95);return {...g,approval,turnout:bound(Math.round(58+Math.abs(approval-50)*.3+(g.id==='retirees'?10:0)),40,85)};});}
 function poll(s){const gs=groups(s),total=gs.reduce((a,g)=>a+g.share*g.turnout,0);return Math.round(gs.reduce((a,g)=>a+g.share*g.turnout*g.approval,0)/total);}
 function changePolicy(s,id,level){const p=policy(id);if(!p||s.ended||s.agendaChoice!==null||level<0||level>4||!Number.isInteger(level))return {ok:false,reason:'This policy cannot be changed now.'};const previous=s.levels[id],steps=Math.abs(level-previous);if(!steps)return {ok:false,reason:'That setting is already in force.'};const cost=steps*(2+(p.lag>1?1:0));if(s.capital<cost)return {ok:false,reason:`Requires ${cost} political capital.`};s.capital-=cost;s.levels[id]=level;s.history.push({quarter:s.quarter,type:'policy',title:p.name,detail:`Set to ${level}/4 · ${cost} political capital`});return {ok:true,cost};}
 function choose(s,index){if(s.ended||s.agendaChoice!==null)return false;const event=AGENDA[s.quarter],choice=event?.choices[index];if(!choice)return false;s.agendaChoice=index;s.effects.push({due:s.quarter+1,delta:choice[1],title:`${event.title}: ${choice[0]}`});s.debt=bound(s.debt+choice[2]*.45,0,250);s.history.push({quarter:s.quarter,type:'agenda',title:event.title,detail:`${choice[0]} · Cost ${choice[2]} budget units; effects arrive next quarter.`});return true;}
 function advance(s){if(s.ended||s.agendaChoice===null)return false;upgrade(s);const report={quarter:s.quarter+1,changes:[],situations:[],budget:null,election:null};s.quarter++;
  for(const promise of s.promises.filter(p=>p.status==='open')){if(s.levels[promise.policy]>=promise.target)report.changes.push(resolvePromise(s,promise,true));else if(s.quarter>=promise.due)report.changes.push(resolvePromise(s,promise,false));}
  for(const group of Object.keys(s.campaign.boosts))s.campaign.boosts[group]*=.75;
  for(const p of P){const current=s.implemented[p.id],target=s.levels[p.id];s.implemented[p.id]=current+(target-current)/p.lag;}
  const incoming=Object.fromEntries(Object.keys(BASE).map(k=>[k,0]));
  for(const p of P)for(const [key,value] of Object.entries(p.effects))incoming[key]+=(s.implemented[p.id]-2)*value;
  for(const effect of s.effects.filter(e=>e.due===s.quarter)){for(const [key,value] of Object.entries(effect.delta))incoming[key]+=value;report.changes.push(effect.title);s.history.push({quarter:s.quarter,type:'effect',title:effect.title,detail:Object.entries(effect.delta).map(([k,v])=>`${METRICS[k]} ${v>0?'+':''}${v}`).join(' · ')});}
  s.effects=s.effects.filter(e=>e.due>s.quarter);
  // Conditions feed back into each other, while inertia stops a lever from solving a crisis instantly.
  incoming.growth+=(s.metrics.jobs-50)*.018-Math.max(0,s.debt-100)*.025;
  incoming.jobs+=(s.metrics.growth-50)*.025;incoming.housing+=(s.metrics.jobs-50)*.025;incoming.health+=(s.metrics.housing-50)*.015;incoming.energy+=(s.metrics.climate-50)*.018;
  incoming.trust+=(s.metrics.jobs-50)*.03+(s.metrics.prices-50)*.025-Math.max(0,s.debt-110)*.02-Math.max(0,-(s.budget?.balance||0)-8)*.03;
  const previous={...s.metrics};for(const key of Object.keys(BASE))s.metrics[key]=bound(Math.round((s.metrics[key]+(BASE[key]+incoming[key]-s.metrics[key])*.36)*10)/10,0,100);
  report.changes.push(...Object.keys(BASE).filter(k=>Math.abs(s.metrics[k]-previous[k])>=1.4).map(k=>`${METRICS[k]} ${s.metrics[k]>previous[k]?'rose':'fell'} to ${Math.round(s.metrics[k])}`));
  const b=budget(s);s.budget=b;s.debt=bound(Math.round((s.debt-b.balance*.3)*10)/10,0,250);report.budget=b;
  const definitions=[['Recession','growth',38,'below'],['Housing emergency','housing',36,'below'],['Overloaded clinics','health',37,'below'],['Power instability','energy',38,'below'],['Trust crisis','trust',35,'below'],['Inflation shock','prices',37,'below'],['Clean-energy transition','climate',65,'above'],['Broad job market','jobs',64,'above']];
  s.situations=definitions.filter(([,k,t,side])=>side==='below'?s.metrics[k]<t:s.metrics[k]>t).map(([name])=>name);if(s.debt>140)s.situations.push('Debt warning');report.situations=[...s.situations];
  const currentPoll=poll(s);s.polls.push({quarter:s.quarter,approval:currentPoll});s.capital=bound(s.capital+(currentPoll>=55?4:currentPoll>=42?3:2)+(s.situations.includes('Trust crisis')?-1:0),0,20);
  if(s.quarter===8){s.midterm={vote:currentPoll,majority:currentPoll>=50};if(!s.midterm.majority)s.capital=bound(s.capital-3,0,20);report.election={type:'midterm',...s.midterm};}
  if(s.quarter===16){s.ended=true;report.election={type:'general',vote:currentPoll,won:currentPoll>=50};}
  s.agendaChoice=null;s.history.push({quarter:s.quarter,type:'report',title:`Quarter ${s.quarter} report`,detail:`Polling ${currentPoll}% · budget ${b.balance>=0?'+':''}${b.balance} · debt ${Math.round(s.debt)}`});return report;}
 return {METRICS,BASE,P,GROUPS,AGENDA,PEOPLE,REGIONS,ISSUES,fresh,migrate,policy,budget,groups,poll,changePolicy,choose,advance,bound,availableSlots,campaignSeason,request,meeting,tripPreview,campaignTrip};
})();
