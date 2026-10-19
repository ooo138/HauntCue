const fs=require('fs'),vm=require('vm'),assert=require('assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').match(/<script>([\s\S]*?)<\/script>/)[1];
new vm.Script(source);
function fn(name){const start=source.indexOf('function '+name+'('),asyncStart=source.slice(Math.max(0,start-6),start)==='async '?start-6:start;let level=0,opened=false;for(let i=source.indexOf('{',start);i<source.length;i++){if(source[i]==='{'){level++;opened=true;}else if(source[i]==='}'&&!--level&&opened)return source.slice(asyncStart,i+1);}throw Error(name);}
const elements=new Map();const el=id=>{if(!elements.has(id))elements.set(id,{value:id==='propName'?'Test prop':'',textContent:'',classList:{add(){},remove(){},toggle(){}},setAttribute(){},focus(){}});return elements.get(id)};
const ctx={document:{getElementById:el,querySelectorAll:()=>[]},location:{search:''},URLSearchParams,URL,TextEncoder,console,localStorage:{setItem(){}},setTimeout,clearTimeout};vm.createContext(ctx);
vm.runInContext(source.slice(0,source.indexOf('function options('))+';this.example=defaults;',ctx);
vm.runInContext(`cfg=clone(defaults);cfg.scenes[0].cues=[];status={armed:false,running:false};stage={};stageDirty=true;drag=null;connected=true;let inventory={activeProp:''},inventoryDirty=true,servoRecording=false,activeOutput=0,sequenceUndo=[],sequenceRedo=[],sequenceSnapshot=null,restoringSequence=false;this.messages=[];this.calls=[];function toast(s,e){messages.push([s,e])}function updateControls(){}function render(){}function renderTimeline(){}function finishServoRecording(){}async function poll(){}function makePropPackage(name,c,l,i){return {config:c,layout:l,inventory:i}}function safeLibraryEdit(){if(status.armed||status.running||mutation)throw Error('Disarm first')}function edited(){captureSequenceHistory();dirty=true}async function api(path){calls.push(path);if(path===failure)throw Error('test disconnect')}let failure='';`,ctx);
for(const name of ['sequenceState','captureSequenceHistory','updateHistoryControls','restoreSequence','addContextCue','saveWholeProp'])vm.runInContext(fn(name),ctx);
vm.runInContext('sequenceSnapshot=sequenceState();cfg.outputs[3].maxOnMs=150;addContextCue(3,2000);',ctx);
assert.equal(vm.runInContext('scene().cues[0].output',ctx),3);assert.equal(vm.runInContext('scene().cues[0].duration',ctx),150);
vm.runInContext('restoreSequence(false)',ctx);assert.equal(vm.runInContext('scene().cues.length',ctx),0);assert.equal(vm.runInContext('cfg.outputs[3].maxOnMs',ctx),150);
vm.runInContext('restoreSequence(true)',ctx);assert.equal(vm.runInContext('scene().cues.length',ctx),1);
vm.runInContext('cfg.motion={a:3,b:4,kind:"pneumatic",deadTime:200};addContextCue(4,2000)',ctx);assert.equal(vm.runInContext('scene().cues.length',ctx),1);assert.match(ctx.messages.at(-1)[0],/dead time/);
(async()=>{vm.runInContext('failure="stage"',ctx);await vm.runInContext('saveWholeProp()',ctx);assert.deepEqual(Array.from(ctx.calls),['config','stage','library']);assert.equal(vm.runInContext('dirty',ctx),false);assert.equal(vm.runInContext('stageDirty',ctx),true);assert.equal(vm.runInContext('inventoryDirty',ctx),false);assert.match(el('saveReport').textContent,/Visual layout: FAILED/);assert.equal(vm.runInContext('mutation',ctx),false);
vm.runInContext('failure="";calls.length=0',ctx);await vm.runInContext('saveWholeProp()',ctx);assert.equal(vm.runInContext('stageDirty',ctx),false);vm.runInContext('status.armed=true;calls.length=0',ctx);await vm.runInContext('saveWholeProp()',ctx);assert.equal(ctx.calls.length,0);
console.log('PASS: contextual CH4 cue, pulse cap, interlock rejection, sequence undo/redo preserves hardware settings, partial-save state, retry and armed-save rejection');})().catch(e=>{console.error(e);process.exitCode=1});


