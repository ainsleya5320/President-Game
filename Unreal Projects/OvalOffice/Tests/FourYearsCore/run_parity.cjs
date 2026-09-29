// Builds the C++ simulation core with the host compiler, generates a fresh reference trace from the
// browser game's JavaScript, and checks that both produce identical results and states.
// Usage: node "Unreal Projects/OvalOffice/Tests/FourYearsCore/run_parity.cjs"   (needs Node and g++ or clang++)
const {execFileSync}=require('node:child_process'),fs=require('node:fs'),os=require('node:os'),path=require('node:path');
const project=path.resolve(__dirname,'..','..'),prototype=path.join(project,'Prototype'),source=path.join(project,'Source','OvalOffice');
const work=fs.mkdtempSync(path.join(os.tmpdir(),'fouryears-parity-')),binary=path.join(work,process.platform==='win32'?'parity.exe':'parity'),trace=path.join(work,'trace.json');
const compiler=['g++','clang++'].find(c=>{try{execFileSync(c,['--version'],{stdio:'ignore'});return true}catch{return false}});
if(!compiler){console.error('No g++ or clang++ found; install one to run the parity test.');process.exit(2);}
// Unreal-like settings: warnings as errors, shadowing checks, no exceptions or RTTI.
execFileSync(compiler,['-std=c++17','-O2','-Wall','-Wextra','-Wshadow','-Werror','-fno-exceptions','-fno-rtti','-I',path.join(source,'Public'),
 path.join(__dirname,'parity_main.cpp'),path.join(source,'Private','FourYears','Core','FourYearsJson.cpp'),path.join(source,'Private','FourYears','Core','FourYearsSim.cpp'),path.join(source,'Private','FourYears','Core','FourYearsElectoral.cpp'),'-o',binary],{stdio:'inherit'});
execFileSync(process.execPath,[path.join(prototype,'parity','make_trace.cjs'),trace],{stdio:'inherit'});
try{execFileSync(binary,[path.join(prototype,'data'),trace],{stdio:'inherit'});}catch{process.exitCode=1;}
fs.rmSync(work,{recursive:true,force:true});
