// Full production-page smoke check. Requires internet for the game's Three.js imports.
const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),http=require('node:http'),vm=require('node:vm');
(async()=>{
 const html=fs.readFileSync(path.join(__dirname,'Four_Years_Prototype.html'));
 const server=http.createServer((req,res)=>{res.writeHead(200,{'Content-Type':'text/html'});res.end(html)});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 let browser;
 try{
  browser=await chromium.launch({headless:true,executablePath:'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe',args:['--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
  const context=await browser.newContext({viewport:{width:1450,height:950}}),page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(e.message));page.on('requestfailed',r=>console.log('Request failed:',r.url(),r.failure()?.errorText));
  const Sim=vm.runInNewContext(fs.readFileSync(path.join(__dirname,'executive-systems.js'),'utf8')+'\n'+fs.readFileSync(path.join(__dirname,'simulation-core.js'),'utf8')+'\nFourYearsSim');
  const old=Sim.fresh();old.version=2;old.quarter=6;for(const key of ['relationships','appointments','promises','goodwill','campaign','executive'])delete old[key];
  await context.addInitScript(save=>{if(!localStorage.getItem('four-years-oval-prototype-v1'))localStorage.setItem('four-years-oval-prototype-v1',save)},JSON.stringify(old));
  await page.goto(`http://127.0.0.1:${server.address().port}/`,{waitUntil:'domcontentloaded'});
  await page.waitForFunction(()=>!document.querySelector('#continue').disabled,null,{timeout:60000});
  await page.locator('#continue').click();await page.locator('#briefing').click();
  await page.locator('#scene').click({position:{x:250,y:400}});await page.keyboard.press('e');
  assert.equal(await page.locator('.person-card').count(),4,'sitting area opens the calendar');
  await page.locator('[data-meet="chen"][data-response="promise"]').click();
  await page.locator('.strategy-tabs [data-tab="congress"]').click();await page.locator('[data-propose="housing"]').click();await page.locator('[data-amend="local"]').click();await page.locator('[data-amend="audit"]').click();await page.locator('#floor-vote').click();assert.match(await page.locator('.project-card').innerText(),/passed/i);
  await page.locator('.strategy-tabs [data-tab="campaign"]').click();await page.locator('#board-campaign').click();
  await page.waitForFunction(()=>document.querySelector('#location-name').textContent==='Air Force One',null,{timeout:60000});
  await page.locator('#take-trip').waitFor({state:'visible'});
  await page.locator('#briefing').click();await page.locator('#scene').click({position:{x:250,y:400}});await page.keyboard.press('e');
  assert.equal(await page.locator('#panel').getAttribute('data-page'),'campaign','conference table opens campaign planning');
  await page.locator('#trip-region').selectOption('coast');await page.locator('#trip-issue').selectOption('housing');
  await page.locator('#take-trip').click();
  assert.equal(await page.locator('#take-trip').isDisabled(),true);
  const saved=JSON.parse(await page.evaluate(()=>localStorage.getItem('four-years-oval-prototype-v1')));
  assert.equal(saved.version,4);assert.equal(saved.quarter,6);assert.equal(saved.promises.length,1);assert.equal(saved.campaign.trips.length,1);assert.equal(saved.location,'aircraft');
  await page.screenshot({path:path.join(__dirname,'../Saved/BrowserPrototype/campaign-room.png')});
  await page.locator('#travel').click();await page.waitForFunction(()=>document.querySelector('#location-name').textContent==='The Oval Office');
  await page.locator('#briefing').click();await page.locator('.strategy-tabs [data-tab="command"]').click();await page.locator('[data-agenda="1"]').click();await page.locator('#advance-quarter').click();
  await page.locator('.strategy-tabs [data-tab="moments"]').click();await page.locator('[data-media="debate"]').click();await page.locator('[data-answer="own"]').click();await page.locator('[data-answer="listen"]').click();
  await page.reload({waitUntil:'domcontentloaded'});await page.waitForFunction(()=>!document.querySelector('#continue').disabled,null,{timeout:60000});await page.locator('#continue').click();
  await page.locator('.strategy-tabs [data-tab="promises"]').click();assert.equal(await page.locator('.promise-card').count(),1);
  assert.deepEqual(errors,[]);
  console.log('Full 3D page passed: save migration, room interactions, housing bill passage, campaign travel, televised debate and save/resume.');
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(e=>{console.error(e);process.exit(1)});
