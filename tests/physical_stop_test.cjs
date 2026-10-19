const fs=require('fs'),vm=require('vm'),assert=require('assert/strict');
const html=fs.readFileSync(__dirname+'/../web/index.html','utf8'),source=html.match(/<script>([\s\S]*?)<\/script>/)[1];
const ctx={document:{getElementById:()=>({})},location:{search:''},URLSearchParams,URL,TextEncoder,console};vm.createContext(ctx);vm.runInContext(source.slice(0,source.indexOf('function options(')),ctx);
assert.equal(vm.runInContext('physicalStopSettings.pin',ctx),16);assert.equal(vm.runInContext('inputPins.includes(16)',ctx),true);assert.equal(vm.runInContext('physicalStopExternalPull(16)',ctx),false);assert.equal(vm.runInContext('physicalStopExternalPull(35)',ctx),true);assert.equal(vm.runInContext('physicalPinConflict(cfg,16)',ctx),false);
vm.runInContext('cfg=normalize(clone(defaults));physicalStopSettings={enabled:true,pin:35,latched:true,open:false,ready:true}',ctx);
assert.equal(vm.runInContext('physicalBlocked()',ctx),true);
assert.equal(vm.runInContext('physicalPinConflict(cfg,35)',ctx),false);
vm.runInContext('physicalStopSettings.latched=false',ctx);assert.equal(vm.runInContext('physicalBlocked()',ctx),false);
vm.runInContext('physicalStopSettings.open=true',ctx);assert.equal(vm.runInContext('physicalBlocked()',ctx),true);
for(const expr of ['cfg.trigger.pin=35','cfg.audio2.enabled=true;cfg.audio2.rx=35','cfg.outputs[0].pin=35','cfg.servo.enabled=true;cfg.servo.pin=35','cfg.storm.enabled=true;cfg.storm.pin=35']){vm.runInContext('cfg=normalize(clone(defaults));'+expr,ctx);assert.throws(()=>vm.runInContext('validate(cfg)',ctx),/reserved/);}
vm.runInContext('cfg=normalize(clone(defaults));physicalStopSettings.enabled=false',ctx);assert.doesNotThrow(()=>vm.runInContext('validate(cfg)',ctx));
const ino=fs.readFileSync(__dirname+'/../firmware/HauntCue/HauntCue.ino','utf8');
for(const path of ['arm','fire','test','servo','light-test']){const start=ino.indexOf('server.on("/api/'+path+'"');assert(start>=0);const end=ino.indexOf('server.on(',start+10);assert.match(ino.slice(start,end<0?undefined:end),/physicalMotionAllowed\(\)/,path);}
assert.match(ino,/prefs\.putString\("physical-stop"/);assert.match(ino,/physicalStop\.reset\(millis\(\)\).*halt\(\)/);
assert.match(ino,/if\(!physicalStop\.blocked\(\)&&!otaBusy&&!configLoadFault\)\{engine\.tick/);
console.log('PASS: latched/open stop blocks movement, reserved trigger/audio pin validation, disabled legacy compatibility, all hardware movement endpoints guarded, independent persistence, reset disarms, engine gate');
