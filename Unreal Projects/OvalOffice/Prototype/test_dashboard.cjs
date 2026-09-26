// Isolated browser save; production markup, styles, embedded portraits and strategy code.
const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),http=require('node:http');
(async()=>{
 const html=fs.readFileSync(path.join(__dirname,'Four_Years_Prototype.html'),'utf8');
 const ui=html.slice(html.indexOf('const ExecutiveSim='),html.indexOf("const canvas=$('scene');"));
 assert.ok(ui.length>10000);
 const pageHtml=html.slice(0,html.indexOf('<script type="module">'))+'<script>const $=id=>document.getElementById(id);function renderWestWingMap(){}function setMode(){}function setAircraftPose(){}function changeLocation(destination){currentLocation=destination;state.location=destination;save()}\n'+ui+'</script></body></html>';
 const server=http.createServer((req,res)=>{res.writeHead(200,{'Content-Type':'text/html'});res.end(pageHtml)});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 let browser;
 try{
  browser=await chromium.launch({headless:true,executablePath:'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe'});
  const page=await browser.newPage({viewport:{width:1600,height:1080}}),errors=[];
  page.on('pageerror',e=>errors.push(e.message));
  await page.goto(`http://127.0.0.1:${server.address().port}/`);
  await page.evaluate(()=>{$('begin').disabled=false});await page.locator('#begin').click();
  await page.locator('#dashboard').click();
  assert.equal(await page.locator('#panel').getAttribute('data-page'),'dashboard');
  assert.equal(await page.locator('.dash-node').count(),12);
  assert.equal(await page.locator('.dash-chamber circle').count(),100);
  assert.equal(await page.locator('.dash-voters button').count(),10);
  assert.equal(await page.locator('.dash-person img').count(),4);
  assert.equal(await page.locator('#dash-approval').textContent(),await page.evaluate(()=>FourYearsSim.poll(state)+'%'));
  assert.equal(await page.locator('#dash-balance').textContent(),await page.evaluate(()=>signed(FourYearsSim.budget(state).balance)+' units'));
  const before=await page.evaluate(()=>JSON.stringify(state));
  await page.locator('#dash-policy').selectOption('housing');
  await page.locator('.dash-node[data-metric="housing"]').press('Enter');
  assert.match(await page.locator('#dash-drivers').innerText(),/Housing access/);
  const driverCount=await page.evaluate(()=>FourYearsSim.P.filter(p=>p.effects.housing).length);
  assert.equal(await page.locator('[data-driver]').count(),driverCount);
  assert.equal(await page.evaluate(()=>JSON.stringify(state)),before,'dashboard exploration never mutates the save');
  await page.locator('#dash-edit-policy').click();assert.match(await page.locator('.policy-detail').innerText(),/Housing/);
  assert.equal(await page.locator('.dashboard-open').count(),0);
  await page.locator('#dashboard').click();await page.locator('[data-dash-region="lakes"]').click();assert.match(await page.locator('.region-detail').innerText(),/Marcus Reed/i);
  await page.locator('#dashboard').click();await page.keyboard.press('Escape');assert.equal(await page.locator('#panel').isHidden(),true);assert.equal(await page.locator('#dashboard').isVisible(),true);
  await page.locator('#dashboard').click();await page.locator('.dash-nav [data-tab="appointments"]').click();
  assert.equal(await page.locator('.person-card img').count(),4);
  await page.evaluate(()=>Promise.all([...document.querySelectorAll('.character-portrait')].map(i=>i.decode())));
  await page.screenshot({path:path.join(__dirname,'../Saved/BrowserPrototype/portraits-calendar.png')});
  await page.locator('.strategy-tabs [data-tab="team"]').click();assert.equal(await page.locator('.opposition-card img').count(),1);
  await page.locator('.strategy-tabs [data-tab="moments"]').click();await page.locator('[data-media="press"]').click();assert.match(await page.locator('.broadcast-speaker img').getAttribute('alt'),/Jordan Ellis/);
  // Replacement identities must not inherit the departed character's photograph.
  assert.match(await page.evaluate(()=>characterPortrait({name:'Dr. Nia Patel',initials:'NP'})),/portrait-letter/);
  await page.evaluate(()=>{
   state=FourYearsSim.fresh();EX.platform(state,['housing','health','trust'],'housing');EX.propose(state,'housing');
   for(let q=0;q<3;q++){FourYearsSim.choose(state,1);FourYearsSim.advance(state)}
   selectedPolicy='housing';dashboardMetric='housing';openPage('dashboard');clearTimeout(toastTimer);$('toast').hidden=true;
  });
  await page.evaluate(()=>Promise.all([...document.querySelectorAll('.character-portrait')].map(i=>i.decode())));
  await page.screenshot({path:path.join(__dirname,'../Saved/BrowserPrototype/dashboard-desktop.png')});
  const yes=await page.evaluate(()=>EX.votes(state,EX.activeBill(state)).reduce((n,b)=>n+b.yes,0));
  assert.equal(await page.locator('.dash-chamber circle[fill="#80d4be"]').count(),yes);
  assert.ok(await page.locator('.dash-trend circle').count()>=3);
  assert.ok(await page.locator('.dash-attention-list button.urgent').count()>0);
  await page.evaluate(()=>{state.location='aircraft';currentLocation='aircraft';openPage('dashboard')});
  await page.locator('#dashboard-close').click();assert.equal(await page.evaluate(()=>state.location),'aircraft');
  await page.locator('#dashboard').click();await page.setViewportSize({width:390,height:844});
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
  assert.equal(await page.evaluate(()=>$('panel').scrollWidth<=$('panel').clientWidth),true,'dashboard does not overflow horizontally');
  await page.locator('.dash-mobile-metrics [data-metric="health"]').click();assert.match(await page.locator('#dash-drivers').innerText(),/Public health/);
  await page.locator('#panel').evaluate(el=>el.scrollTop=0);
  await page.screenshot({path:path.join(__dirname,'../Saved/BrowserPrototype/dashboard-mobile.png')});
  // The completed term remains readable and does not access a nonexistent agenda.
  await page.evaluate(()=>{state.quarter=16;state.ended=true;openPage('dashboard')});
  assert.match(await page.locator('.dash-top').innerText(),/Final record/);
  assert.deepEqual(errors,[]);
  console.log('Dashboard passed: live values, all 12 conditions, 100 seats, actual policy links, navigation, portraits, save isolation, mobile and completed term.');
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(e=>{console.error(e);process.exit(1)});
