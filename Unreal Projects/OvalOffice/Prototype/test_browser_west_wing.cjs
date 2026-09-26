// Production Three.js scenes in an isolated browser profile; no user save is touched.
const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),http=require('node:http');
(async()=>{
 const hooks=`window.wingTest={snapshot:()=>({location:currentLocation,position:camera.position.toArray(),clear:canWalk(camera.position.x,camera.position.z),quarter:state.quarter,meshes:westWingModel?.children.length}),pose:setWestWingPose,place:placeCamera,walk:westWingCanWalk};`;
 const html=fs.readFileSync(path.join(__dirname,'Four_Years_Prototype.html'),'utf8').replace(/<\/script>\s*<\/body>/,hooks+'</script>\n</body>');
 assert.ok(html.includes(hooks));const server=http.createServer((req,res)=>{res.writeHead(200,{'Content-Type':'text/html'});res.end(html)});await new Promise(r=>server.listen(0,'127.0.0.1',r));let browser;
 try{
  browser=await chromium.launch({headless:true,executablePath:'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe',args:['--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
  const page=await browser.newPage({viewport:{width:1500,height:1000}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto(`http://127.0.0.1:${server.address().port}/`,{waitUntil:'domcontentloaded'});await page.waitForFunction(()=>!!window.wingTest,null,{timeout:90000});
  await page.locator('#begin').click();await page.locator('#westwing').click();await page.waitForFunction(()=>window.wingTest.snapshot().location==='westwing',null,{timeout:90000});
  assert.equal(await page.locator('#wing-room option').count(),8);assert.ok((await page.evaluate(()=>wingTest.snapshot())).meshes>20);
  const quarter=(await page.evaluate(()=>wingTest.snapshot())).quarter;
  const out=path.join(__dirname,'../Saved/BrowserPrototype');fs.mkdirSync(out,{recursive:true});
  await page.screenshot({path:path.join(out,'west-wing-gallery.png')});
  await page.locator('#scene').click({position:{x:550,y:450}});const before=await page.evaluate(()=>wingTest.snapshot().position);
  await page.keyboard.down('w');await page.waitForFunction(p=>{const a=wingTest.snapshot().position;return Math.hypot(a[0]-p[0],a[2]-p[2])>.30},before,{timeout:20000});await page.keyboard.up('w');const after=await page.evaluate(()=>wingTest.snapshot());assert.ok(after.clear);
  // Walk into a solid wall; the camera must stop on the near side.
  await page.evaluate(()=>wingTest.place([3.8,1.65,-15],[5,1.65,-15]));await page.keyboard.down('w');await page.waitForFunction(()=>wingTest.snapshot().position[0]>4.0,null,{timeout:20000});await page.waitForTimeout(1500);await page.keyboard.up('w');assert.ok((await page.evaluate(()=>wingTest.snapshot())).position[0]<4.23);
  for(const id of ['cabinet','roosevelt','study','lobby','press','colonnade','garden']){
   await page.locator('#wing-room').selectOption(id);assert.ok((await page.evaluate(()=>wingTest.snapshot())).clear,id+' shortcut is clear');
   if(['cabinet','press','garden'].includes(id))await page.screenshot({path:path.join(out,'west-wing-'+id+'.png')});
  }
  await page.locator('#wing-map').click();assert.equal(await page.locator('.wing-destinations button').count(),8);await page.screenshot({path:path.join(out,'west-wing-map.png')});
  await page.locator('.wing-destinations [data-wing-place="cabinet"]').click();await page.locator('#scene').click({position:{x:550,y:450}});await page.keyboard.press('e');assert.equal(await page.locator('#panel').getAttribute('data-page'),'team');await page.locator('#briefing').click();
  await page.locator('#dashboard').click();assert.match(await page.locator('#dashboard-close').innerText(),/West Wing/);await page.locator('#dashboard-close').click();
  await page.locator('#wing-map').click();await page.locator('#wing-back-oval').click();await page.waitForFunction(()=>wingTest.snapshot().location==='oval');await page.locator('#briefing').click();assert.equal(await page.locator('#panel').getAttribute('data-page'),'command','Wing map must not move the player outside the Oval after returning');await page.locator('#westwing').click();await page.waitForFunction(()=>wingTest.snapshot().location==='westwing');
  await page.reload({waitUntil:'domcontentloaded'});await page.waitForFunction(()=>!!window.wingTest,null,{timeout:90000});await page.locator('#continue').click();await page.waitForFunction(()=>wingTest.snapshot().location==='westwing',null,{timeout:90000});
  if(await page.locator('#panel').isVisible())await page.locator('#briefing').click();await page.locator('#walk').click();await page.locator('#scene').click({position:{x:550,y:450}});await page.keyboard.press('e');await page.waitForFunction(()=>wingTest.snapshot().location==='oval');
  assert.equal((await page.evaluate(()=>wingTest.snapshot())).quarter,quarter);
  await page.locator('#westwing').click();await page.waitForFunction(()=>wingTest.snapshot().location==='westwing');await page.locator('#travel').click();await page.waitForFunction(()=>wingTest.snapshot().location==='aircraft',null,{timeout:90000});await page.locator('#travel').click();await page.waitForFunction(()=>wingTest.snapshot().location==='oval');
  assert.deepEqual(errors,[]);console.log('West Wing production browser passed: loading, WASD, walls, all room shortcuts, floor map, Cabinet interaction, dashboard, save/resume, Oval doorway and Air Force One travel.');
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exit(1)});
