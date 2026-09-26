const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const http=require('node:http');

// Exercise the real strategy code in a browser without requiring the remote Three.js CDN.
// 3D movement and aircraft collision have their separate regression check.
(async()=>{
 const server=http.createServer((req,res)=>{res.writeHead(200,{'Content-Type':'text/html'});res.end('<!doctype html><title>Strategy test</title>')});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 const executablePath=['C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe','C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe'].find(fs.existsSync);
 const browser=await chromium.launch({headless:true,executablePath});
 try{
  const page=await browser.newPage({viewport:{width:1450,height:900}});
  const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto(`http://127.0.0.1:${server.address().port}/`,{waitUntil:'domcontentloaded'});
  const template=fs.readFileSync(path.join(__dirname,'office-game.template.html'),'utf8');
  const css=template.match(/<style>([\s\S]*?)<\/style>/)[1].replace('__STRATEGY_CSS__',fs.readFileSync(path.join(__dirname,'strategy-ui.css'),'utf8')).replace('__DASHBOARD_CSS__',fs.readFileSync(path.join(__dirname,'dashboard-ui.css'),'utf8'));
  await page.setContent(`<!doctype html><style>${css}</style><main class="shell"><header class="mast"><div class="brand">FOUR YEARS<small id="location-name">The Oval Office</small></div><div class="mast-right"><div class="month" id="date"></div><div class="stats" id="stats"></div></div></header><div class="stage"><div class="controls"><button id="briefing" class="chip">Open strategy</button><button id="policies" class="chip">Policies</button><button id="voters" class="chip">Voters</button><button id="budget" class="chip">Budget</button><button id="journal" class="chip">Journal</button></div><aside class="panel" id="panel"></aside><div class="toast" id="toast" hidden></div><div id="start"><button id="begin">Begin</button><button id="continue" hidden>Continue</button></div></div></main>`);
  await page.addScriptTag({content:"const $=id=>document.getElementById(id); function setMode(){} function setAircraftPose(){} function changeLocation(destination){currentLocation=destination;state.location=destination;save()}"});
  await page.addScriptTag({path:path.join(__dirname,'executive-systems.js')});
  await page.addScriptTag({path:path.join(__dirname,'simulation-core.js')});
  await page.addScriptTag({path:path.join(__dirname,'strategy-ui.js')});
  await page.addScriptTag({path:path.join(__dirname,'executive-ui.js')});
  await page.addScriptTag({content:fs.readFileSync(path.join(__dirname,'dashboard-ui.js'),'utf8').replace('__CHARACTER_PORTRAITS__','{}')});
  await page.locator('#begin').click();
  await page.locator('.strategy-tabs [data-tab="legacy"]').click();await page.locator('#set-platform').click();assert.equal(await page.locator('#set-platform').count(),0);
  await page.locator('#policies').click();
  assert.equal(await page.locator('.policy-row').count(),26);
  await page.locator('#more-policy').click();
  assert.match(await page.locator('.policy-detail').innerText(),/Strength 3\/4/);
  await page.locator('#voters').click();assert.equal(await page.locator('.voter-card').count(),10);
  await page.locator('#budget').click();assert.match(await page.locator('.budget-total').innerText(),/Revenue/);
  await page.locator('.strategy-tabs [data-tab="appointments"]').click();
  assert.equal(await page.locator('.person-card').count(),4);
  await page.locator('[data-meet="chen"][data-response="promise"]').click();
  assert.match(await page.locator('.appointment-banner').innerText(),/1 appointment remaining/);
  await page.locator('.strategy-tabs [data-tab="promises"]').click();
  await page.locator('[data-fulfill="housing"]').click();await page.locator('#more-policy').click();
  await page.locator('.strategy-tabs [data-tab="congress"]').click();await page.locator('[data-propose="housing"]').click();
  await page.locator('[data-amend="local"]').click();await page.locator('[data-amend="audit"]').click();
  await page.screenshot({path:path.join(__dirname,'../Saved/BrowserPrototype/executive-congress.png')});
  await page.locator('#floor-vote').click();assert.match(await page.locator('.project-card').innerText(),/passed/i);
  await page.locator('.strategy-tabs [data-tab="nation"]').click();assert.equal(await page.locator('[data-region]').count(),4);await page.locator('[data-region="lakes"]').click();assert.match(await page.locator('.region-detail').innerText(),/Marcus Reed/i);
  await page.locator('.strategy-tabs [data-tab="team"]').click();assert.equal(await page.locator('.person-card').count(),4);
  await page.locator('.strategy-tabs [data-tab="moments"]').click();await page.locator('[data-media="press"]').click();await page.locator('[data-answer="timeline"]').click();assert.match(await page.locator('.broadcast-card').innerText(),/missed deadlines/);await page.locator('[data-answer="transparent"]').click();
  await page.locator('#briefing').click();await page.locator('#briefing').click();
  await page.locator('[data-tab="command"]').click();await page.locator('[data-agenda="0"]').click();
  await page.locator('#advance-quarter').click();assert.match(await page.locator('#date').innerText(),/Quarter 2/i);
  assert.match(await page.locator('#panel').innerText(),/Promise kept/);
  for(let q=1;q<6;q++){await page.locator('.strategy-tabs [data-tab="command"]').click();await page.locator('[data-agenda="1"]').click();await page.locator('#advance-quarter').click();}
  await page.locator('.strategy-tabs [data-tab="campaign"]').click();
  assert.equal(await page.locator('#take-trip').isDisabled(),true);
  await page.locator('#board-campaign').click();
  await page.locator('#trip-region').selectOption('plains');await page.locator('#trip-issue').selectOption('energy');await page.locator('#trip-person').selectOption('ruiz');
  await page.locator('#take-trip').click();assert.equal(await page.locator('#take-trip').isDisabled(),true);
  assert.match(await page.locator('#panel').innerText(),/Campaign trail/);
  await page.screenshot({path:path.join(__dirname,'../Saved/BrowserPrototype/strategy-preview.png')});
  await page.setViewportSize({width:390,height:844});await page.locator('.strategy-tabs [data-tab="nation"]').click();
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=window.innerWidth),true,'mobile layout stays inside the viewport');
  await page.screenshot({path:path.join(__dirname,'../Saved/BrowserPrototype/executive-mobile.png')});
  assert.deepEqual(errors,[]);
  console.log('Browser strategy passed: mandate, Congress, regional map, team, press follow-up, promises, campaign and mobile layout.');
 }finally{await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exit(1)});
