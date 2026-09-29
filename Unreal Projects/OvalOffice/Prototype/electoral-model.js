/* County electorate: measured geography and marginals; authored, transparent game behavior.
   This is a fictional two-party simulation, not an election forecast. */
const CountyElectorate=(()=>{
 const D=FourYearsData['us-electorate'],clamp=(x,a=0,b=100)=>Math.max(a,Math.min(b,x)),r=x=>Math.round(x*1000)/1000;
 const cultures=Object.fromEntries(D.cultures.map(c=>[c.id,c]));
 let key='',cached=null;
 function fingerprint(s,signal){return JSON.stringify([s.metrics,s.levels,s.implemented,s.executive?.regions,s.executive?.opposition,s.electoral,s.quarter,s.seed,signal]);}
 function view(s,signal=51){
  const k=fingerprint(s,signal);if(k===key&&cached)return cached;key=k;
  const party=s.electoral?.party||'',direction=party==='republican'?-1:1,rows=[],states=Object.fromEntries(D.states.map(x=>[x.id,{...x,votes:0,supportVotes:0,approvalSum:0,adults:0,turnoutSum:0,counties:0}]));
  const nations=Object.fromEntries(D.cultures.map(c=>[c.id,{id:c.id,name:c.name,adults:0,votes:0,supportVotes:0,approvalSum:0}]));
  let nationalVotes=0,nationalSupport=0,nationalApproval=0,adults=0,turnoutSum=0;
  for(const c of D.counties){
   const local=s.executive?.regions?.[c.region]||{},m=s.metrics;
   let service=0,autonomy=0,ecology=0;for(const [id,w] of Object.entries(c.cultures)){service+=cultures[id].service*w;autonomy+=cultures[id].autonomy*w;ecology+=cultures[id].ecology*w;}
   const lev=id=>(s.implemented?.[id]??s.levels[id]??2)-2;
   const policy=service*(lev('clinic')+lev('insurance')+lev('teacher')+lev('tax'))*.48+ecology*(lev('carbon')+lev('renewables')-lev('fossil'))*.55-autonomy*lev('surveillance')*.6-c.mining*(lev('carbon')-lev('fossil'))*4-c.manufacturing*lev('trade')*2;
   let field=0;for(const e of s.electoral?.organizing||[]){const decay=Math.pow(.65,Math.max(0,s.quarter-e.quarter));if(e.county===c.id)field+=2.5*decay;else if(e.state===c.state)field+=.18*decay;}
   field=clamp(field,0,5);
   const localEffect=((local.housing??m.housing)-m.housing+(local.jobs??m.jobs)-m.jobs+(local.health??m.health)-m.health+(local.energy??m.energy)-m.energy)/4*.3+(local.goodwill||0)*.55;
   let vote=0,turn=0,approval=0;const cohorts=[];
   for(let a=0;a<2;a++)for(let e=0;e<2;e++)for(let h=0;h<2;h++){
    const w=(a?c.senior:1-c.senior)*(e?c.college:1-c.college)*(h?c.homeowner:1-c.homeowner);
    let perf=0,total=0;for(const [issue,weight] of Object.entries(c.weights)){
     const factor=issue==='health'?(a?1.65:.85):issue==='housing'?(h?.8:1.7):issue==='schools'?(a?.65:1.2):issue==='climate'?(e?1.3:.85):1;
     perf+=(m[issue]-50)*weight*factor;total+=weight*factor;
    }
    const mood=clamp(51+perf/total*.9+(signal-51)*.35+policy+localEffect,3,97);
    // Demographic categories overlap. Race is descriptive only, never a fixed voting coefficient.
    const prior=party?(direction===1?c.dem2024:100-c.dem2024):50;
    const response=.7+(1-Math.abs(prior-50)/50)*.35;
    const support=clamp(prior+2+(mood-50)*.42*response+field,1,99);
    const baseline=clamp(c.baseTurnout+(a-c.senior)*.08+(e-c.college)*.06+(h-c.homeowner)*.03,.15,.96);
    const participation=clamp(baseline+Math.abs(mood-50)*.0006+(m.trust-53)*.001+field*.008,.12,.98);
    const ballots=c.ballots*w*participation/baseline;
    vote+=ballots*support/100;turn+=ballots;approval+=w*mood;
    cohorts.push({name:(a?'65+':'18-64')+' / '+(e?'college':'no BA')+' / '+(h?'owner':'renter'),share:r(w*100),approval:r(mood),support:r(support),turnout:r(participation*100)});
   }
   const turnout=clamp(c.baseTurnout*turn/c.ballots*100,0,100),support=vote/turn*100;
   const row={id:c.id,state:c.state,approval:r(approval),support:r(support),turnout:r(turnout),votes:r(turn),swing:r(support-(party==='republican'?100-c.dem2024:party?c.dem2024:50)),field:r(field),cohorts};rows.push(row);
   const st=states[c.state];st.votes+=turn;st.supportVotes+=vote;st.approvalSum+=approval*c.adults;st.adults+=c.adults;st.turnoutSum+=turnout*c.adults;st.counties++;
   for(const [id,w] of Object.entries(c.cultures)){const n=nations[id];n.adults+=c.adults*w;n.votes+=turn*w;n.supportVotes+=vote*w;n.approvalSum+=approval*c.adults*w;}
   nationalVotes+=turn;nationalSupport+=vote;nationalApproval+=approval*c.adults;adults+=c.adults;turnoutSum+=turnout*c.adults;
  }
  let points=0,opponent=0,tossup=0;const units=[];
  for(const st of Object.values(states)){
   st.support=r(st.supportVotes/st.votes*100);st.approval=r(st.approvalSum/st.adults);st.turnout=r(st.turnoutSum/st.adults);st.margin=r((st.support-50)*2);
   const count=(st.id==='ME'||st.id==='NE')?2:st.ev;
   units.push({id:st.id,name:st.name,approval:st.support,points:count,won:st.support>50});
   if(st.support>50)points+=count;else if(st.support<50)opponent+=count;else tossup+=count;
   st.won=st.support>50;st.points=st.won?count:0;
  }
  const districts=D.districts.map(d=>{
   const st=states[d.state],base=party==='republican'?100-d.dem2024:party?d.dem2024:50,stateBase=party==='republican'?100-st.dem2024:party?st.dem2024:50;
   const support=r(clamp(base+st.support-stateBase,1,99)),won=support>50;
   if(won){points++;st.points++;}else if(support<50)opponent++;else tossup++;
   const u={id:d.id,name:d.id+' district',approval:support,points:1,won};units.push(u);return {...d,support,won};
  });
  const nationList=Object.values(nations).map(n=>({...n,support:r(n.supportVotes/n.votes*100),approval:r(n.approvalSum/n.adults)}));
  cached={party,approval:r(nationalApproval/adults),support:r(nationalSupport/nationalVotes*100),turnout:r(turnoutSum/adults),adults,points,opponent,tossup,won:points>=270,tie:points<270&&opponent<270,states:Object.values(states),districts,counties:rows,nations:nationList,units};return cached;
 }
 function election(v){return {points:v.points,regions:v.units,won:v.won,tie:v.tie,vote:v.support,total:538,needed:270,opponent:v.opponent};}
 function chooseParty(s,party){if(!['democrat','republican'].includes(party)||s.electoral?.party||s.ended)return {ok:false,reason:'Choose your party once per presidency.'};s.electoral={party,organizing:[],chosen:s.quarter};return {ok:true,detail:(party==='democrat'?'Democratic':'Republican')+' administration selected. Your existing decisions and progress are preserved.'};}
 function organize(s,id){const c=D.counties.find(c=>c.id===id);if(!c||!s.electoral?.party||s.ended||s.agendaChoice!==null)return {ok:false,reason:'Choose your party and keep the quarterly decision open.'};if(s.capital<2||s.electoral.organizing.some(e=>e.quarter===s.quarter))return {ok:false,reason:'One field operation each quarter, costing 2 political capital.'};s.capital-=2;s.electoral.organizing.push({county:id,state:c.state,quarter:s.quarter});const detail='Field organizers arrive in '+c.name+', '+c.state+'. Local enthusiasm and turnout improve; the effect fades each quarter.';s.history.push({quarter:s.quarter,type:'organizing',title:'Local organizing',detail});return {ok:true,detail};}
 return {view,election,chooseParty,organize,DATA:D};
})();
