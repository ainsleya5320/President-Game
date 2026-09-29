// Node loader for the rules tests: the same data bundle and scripts the builder embeds in the playable HTML.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
function dataScript(dir=path.join(__dirname,'data')){
 const data=Object.fromEntries(fs.readdirSync(dir).filter(f=>f.endsWith('.json')).sort().map(f=>[path.basename(f,'.json'),JSON.parse(fs.readFileSync(path.join(dir,f),'utf8'))]));
 return `const FourYearsData=${JSON.stringify(data)};`;
}
function loadSimulation(data){
 const read=f=>fs.readFileSync(path.join(__dirname,f),'utf8');
 const bundle=data?`const FourYearsData=${JSON.stringify(data)};`:dataScript();
 return vm.runInNewContext([bundle,read('electoral-model.js'),read('executive-systems.js'),read('simulation-core.js'),'FourYearsSim'].join('\n'));
}
module.exports={dataScript,loadSimulation};
