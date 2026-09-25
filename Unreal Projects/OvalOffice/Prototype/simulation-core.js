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
 function fresh(){const levels=Object.fromEntries(P.map(p=>[p.id,2]));return {version:2,quarter:0,location:'oval',levels,implemented:{...levels},metrics:{...BASE},capital:12,debt:72,budget:null,agendaChoice:null,history:[],effects:[],situations:[],polls:[],midterm:null,ended:false,legacy:null,seed:6137};}
 function migrate(old){if(!old||typeof old!=='object')return fresh();if(old.version===2)return old;const s=fresh();s.location=old.location==='aircraft'?'aircraft':'oval';s.quarter=bound(Math.floor((old.month||0)/3),0,15);s.metrics.trust=bound(Number(old.trust)||53);s.metrics.growth=bound(Number(old.fiscal)||50);s.capital=bound(Math.round((Number(old.capital)||7)*1.5),0,20);s.legacy={months:old.month||0,history:old.history||[],trust:old.trust,fiscal:old.fiscal};s.history.push({quarter:s.quarter,type:'legacy',title:'First-year record preserved',detail:`${s.legacy.months} monthly briefings from the earlier prototype are archived below.`});return s;}
 function policy(id){return P.find(p=>p.id===id)}
 function budget(s){let revenue=87,spending=85;for(const p of P){const contribution=(s.levels[p.id]-2)*p.cost;if(p.cost<0)revenue-=contribution;else spending+=contribution;}revenue+=(s.metrics.growth-50)*.2;spending+=s.debt*.045;return {revenue:Math.round(revenue),spending:Math.round(spending),balance:Math.round(revenue-spending),interest:Math.round(s.debt*.045)};}
 function groups(s){return GROUPS.map(g=>{let weighted=0,total=0;for(const [key,w] of Object.entries(g.priorities)){weighted+=(s.metrics[key]-50)*w;total+=w;}let stance=0;for(const id of g.fav)stance+=(s.levels[id]-2)*1.8;for(const id of g.opp)stance-=(s.levels[id]-2)*1.8;const approval=bound(Math.round(51+weighted/Math.max(1,total)*.85+stance),5,95);return {...g,approval,turnout:bound(Math.round(58+Math.abs(approval-50)*.3+(g.id==='retirees'?10:0)),40,85)};});}
 function poll(s){const gs=groups(s),total=gs.reduce((a,g)=>a+g.share*g.turnout,0);return Math.round(gs.reduce((a,g)=>a+g.share*g.turnout*g.approval,0)/total);}
 function changePolicy(s,id,level){const p=policy(id);if(!p||s.ended||s.agendaChoice!==null||level<0||level>4||!Number.isInteger(level))return {ok:false,reason:'This policy cannot be changed now.'};const previous=s.levels[id],steps=Math.abs(level-previous);if(!steps)return {ok:false,reason:'That setting is already in force.'};const cost=steps*(2+(p.lag>1?1:0));if(s.capital<cost)return {ok:false,reason:`Requires ${cost} political capital.`};s.capital-=cost;s.levels[id]=level;s.history.push({quarter:s.quarter,type:'policy',title:p.name,detail:`Set to ${level}/4 · ${cost} political capital`});return {ok:true,cost};}
 function choose(s,index){if(s.ended||s.agendaChoice!==null)return false;const event=AGENDA[s.quarter],choice=event?.choices[index];if(!choice)return false;s.agendaChoice=index;s.effects.push({due:s.quarter+1,delta:choice[1],title:`${event.title}: ${choice[0]}`});s.debt=bound(s.debt+choice[2]*.45,0,250);s.history.push({quarter:s.quarter,type:'agenda',title:event.title,detail:`${choice[0]} · Cost ${choice[2]} budget units; effects arrive next quarter.`});return true;}
 function advance(s){if(s.ended||s.agendaChoice===null)return false;const report={quarter:s.quarter+1,changes:[],situations:[],budget:null,election:null};s.quarter++;
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
 return {METRICS,BASE,P,GROUPS,AGENDA,fresh,migrate,policy,budget,groups,poll,changePolicy,choose,advance,bound};
})();
