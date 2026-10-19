const fs=require('fs'),vm=require('vm'),assert=require('assert/strict');
const source=fs.readFileSync(__dirname+'/../web/index.html','utf8').match(/<script>([\s\S]*?)<\/script>/)[1];new vm.Script(source);
function fn(name){const start=source.indexOf('function '+name+'(');let level=0;for(let i=source.indexOf('{',start);i<source.length;i++){if(source[i]==='{')level++;else if(source[i]==='}'&&!--level)return source.slice(start,i+1);}throw Error(name);}
const ctx={document:{getElementById:()=>({})},location:{search:''},URLSearchParams,URL,TextEncoder,console};vm.createContext(ctx);vm.runInContext(source.slice(0,source.indexOf('function options(')),ctx);
vm.runInContext('cfg=normalize(clone(defaults));cfg.scenes.push({...clone(cfg.scenes[0]),name:"Ambient",cues:[]});cfg.behavior.idle=1;timelineMode="ambient";',ctx);
assert.equal(vm.runInContext('editingSceneIndex()',ctx),1);assert.equal(vm.runInContext('cfg.selected',ctx),0);
vm.runInContext('scene().cues.push({output:3,start:0,duration:100});',ctx);assert.equal(vm.runInContext('cfg.scenes[0].cues.length',ctx),5);assert.equal(vm.runInContext('cfg.scenes[1].cues.length',ctx),1);
assert.equal(vm.runInContext('editorPreviewConfig().selected',ctx),1);assert.equal(vm.runInContext('cfg.selected',ctx),0);
vm.runInContext(fn('relocateCue'),ctx);vm.runInContext('cfg=relocateCue(cfg,editingSceneIndex(),0,4,200,100)',ctx);assert.equal(vm.runInContext('cfg.scenes[1].cues[0].output',ctx),4);assert.equal(vm.runInContext('cfg.selected',ctx),0);
vm.runInContext('timelineMode="scare";activeEditScene=1',ctx);assert.equal(vm.runInContext('editingSceneIndex()',ctx),0);
vm.runInContext('cfg.scenes.push({...clone(cfg.scenes[0]),name:"Alternative scare"});activeEditScene=2',ctx);assert.equal(vm.runInContext('editingSceneIndex()',ctx),2);assert.equal(vm.runInContext('cfg.selected',ctx),0);
assert.equal(vm.runInContext('validate(cfg).behavior.idle',ctx),1);
vm.runInContext(fn('sequenceState'),ctx);assert.equal(vm.runInContext('sequenceState().idle',ctx),1);
vm.runInContext('timelineMode="ambient";cfg.behavior.idle=-1',ctx);assert.equal(vm.runInContext('editingSceneIndex()',ctx),2);assert.equal(vm.runInContext('cfg.selected',ctx),0);
console.log('PASS: ambient editing and cue relocation preserve triggered scare and its cues, local preview targets viewed scene, scare browsing does not reassign triggers, role metadata survives validation/history, disabled ambient fallback');
