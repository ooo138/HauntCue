const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const html = fs.readFileSync(__dirname+'/../web/index.html','utf8');
const source = html.match(/<script>([\s\S]*?)<\/script>/)[1];
new vm.Script(source); // parse entire application
const mock = { getElementById:()=>({}),querySelectorAll:()=>[] };
const context = {document:mock,location:{search:'?demo'},URLSearchParams,URL,TextEncoder,console};
vm.createContext(context);
// Test the real validator/defaults, without substituting a parallel implementation.
const prefix = source.slice(0,source.indexOf('function options('));
vm.runInContext(prefix+';this.check=validate;this.example=defaults;this.preset=skeletonPreset;this.search=soundSearchURL;this.catalog=checkCatalog;',context);
vm.runInContext('this.servoAngle=servoAngleAt;this.recorded=recordedMoves;',context);
vm.runInContext('this.reset=resetPins;this.peer=peerURL;',context);
vm.runInContext('this.boards=boardReferences;this.positions=headerLocations;this.boardSvg=boardSVG;this.wires=wireList;',context);
const c=()=>JSON.parse(JSON.stringify(context.example));
vm.runInContext(source.slice(source.indexOf('function wizardResize('),source.indexOf('function wizardField('))+';this.resizeWizard=wizardResize;',context);
const wizardOriginal=c(), wizardReduced=context.resizeWizard(wizardOriginal,2);
assert.equal(wizardOriginal.outputs.length,6);
assert.equal(wizardReduced.config.outputs.length,2);
assert.equal(wizardReduced.removed,1);
assert(wizardReduced.config.scenes.every(sc=>sc.cues.every(q=>q.output<2)));
assert.throws(()=>context.resizeWizard(c(),0));
assert.throws(()=>context.resizeWizard(c(),7));
const withoutSensor=c();withoutSensor.trigger.enabled=false;
assert.equal(context.check(withoutSensor).trigger.enabled,false);
withoutSensor.trigger.enabled='false';assert.throws(()=>context.check(withoutSensor));
assert.equal(context.check(c()).scenes.length,1);
assert.equal(c().outputs.length,6);
const devkit=context.boards.find(b=>b.id==='devkitc');const doit=context.boards.find(b=>b.id==='doit');
assert.equal(context.positions(devkit,25)[0].row,9);assert.equal(context.positions(devkit,25)[0].header,'J2');assert.equal(context.positions(devkit,18)[0].header,'J3');assert.equal(context.positions(devkit,18)[0].row,9);
assert.equal(context.positions(doit,25)[0].row,8);assert.equal(context.positions(doit,18)[0].row,7);
assert.equal(context.boards.filter(b=>!b.supported).length,6);
let mapped=context.boardSvg(devkit,context.wires(context.check(c())),false);assert(!mapped.includes('Eyes / lighting →'));
mapped=context.boardSvg(devkit,context.wires(context.check(c())),true);assert(mapped.includes('GPIO25'));
assert(html.includes('TWO rows on each side'));assert(html.includes('Controller features'));
let fresh=context.reset(c());assert.equal(fresh.outputs[5].pin,23);assert.equal(fresh.servo.pin,13);assert.equal(fresh.trigger.pin,32);
assert.equal(context.peer({ip:'192.168.1.42',port:80}),'http://192.168.1.42/');assert.throws(()=>context.peer({ip:'999.1.2.3',port:80}));assert.throws(()=>context.peer({ip:'javascript:alert(1)',port:80}));
let skeleton=context.preset(c());assert.equal(skeleton.scenes[0].duration,26000);assert.equal(skeleton.scenes[0].cues.length,22);assert.equal(skeleton.repeat.enabled,false);assert.equal(skeleton.outputs[0].pin,25);
let servo=context.check(c());servo.servo.enabled=true;servo.servo.pin=25;assert.throws(()=>context.check(servo));servo.servo.pin=13;
servo.scenes[0].servoMoves=[{start:100,duration:200,angle:150},{start:500,duration:200,angle:30}];context.check(servo);
assert.equal(context.servoAngle(servo.scenes[0],200,90),120);assert.equal(context.servoAngle(servo.scenes[0],600,90),90);
servo.scenes[0].servoMoves[1].start=200;assert.throws(()=>context.check(servo));
const recorded=context.recorded([{time:0,angle:90},{time:500,angle:120},{time:1000,angle:60}]);assert.equal(recorded.length,3);assert.equal(recorded[1].start,1);assert.equal(recorded[2].duration,500);
let bad=JSON.parse(JSON.stringify(skeleton));bad.scenes[0].cues.push({output:2,start:500,duration:100});assert.throws(()=>context.check(bad));
bad=JSON.parse(JSON.stringify(skeleton));bad.scenes[0].cues[0].duration=300;assert.throws(()=>context.check(bad));
assert.match(decodeURIComponent(context.search('freesound','stomach rumble')),/Creative Commons 0/);
assert.throws(()=>context.check(null));assert.throws(()=>context.search('unknown','test'));
context.catalog([{title:'Rumble',track:2,kind:'scare',url:'https://freesound.org/',license:'CC0'}]);
assert.throws(()=>context.catalog([{title:'bad',track:2,kind:'scare',url:'javascript:alert(1)',license:'CC0'}]));
let v=c(); v.outputs[1].pin=25; assert.throws(()=>context.check(v));
v=c(); v.trigger.pin=34; assert.throws(()=>context.check(v));v.trigger.pull=0;context.check(v);
v=c(); v.audio.enabled=true;v.audio.tx=25;assert.throws(()=>context.check(v));
v=c();v.scenes[0].cues[0].duration=11000;assert.throws(()=>context.check(v));
v=c();v.scenes[0].cues[0].duration=-1;assert.throws(()=>context.check(v));
v=c();v.scenes[0].cues[0].start=NaN;assert.throws(()=>context.check(v));
v=c();v.outputs[0].activeHigh='false';assert.throws(()=>context.check(v));
v=c();v.selected=8;assert.throws(()=>context.check(v));
v=c();v.scenes[0].name='💀'.repeat(9);assert.throws(()=>context.check(v));
v=c();v.scenes[0].cues=[];context.check(v);
const ids=[...html.matchAll(/\bid="([^"]+)"/g)].map(m=>m[1]);
assert.equal(new Set(ids).size,ids.length,'duplicate static DOM IDs');
console.log('PASS: full JS syntax, actual config validator, pin conflicts, input pulls, audio conflicts, cue bounds, malformed imports, UTF-8 names, empty scenes, DOM IDs');

const photo=context.boards.find(b=>b.id==='photo-mini'); for(const [pin,side,row] of [[25,'leftOuter',3],[26,'rightInner',3],[27,'leftOuter',2],[18,'rightInner',4],[19,'rightInner',5],[23,'rightInner',6],[32,'leftOuter',4],[13,'rightInner',9],[21,'leftInner',4],[22,'leftInner',3]]) {const found=context.positions(photo,pin);assert.equal(found.length,1);assert.equal(found[0].side,side);assert.equal(found[0].row,row);}assert.equal(context.positions(photo,13)[0].label,'TCK');assert(context.boardSvg(photo,context.wires(context.check(c())),true).includes('UNDERSIDE FACING YOU'));console.log('PASS: photo four-row GPIO positions and TCK servo alias');
const stageCtx={document:{getElementById:()=>({})},location:{search:'?demo'},URLSearchParams,URL,TextEncoder};vm.createContext(stageCtx);vm.runInContext(source.slice(0,source.indexOf('function options('))+';this.stageValidate=validateStage;this.stageDefault=defaultStage;this.signal=stageSignal;this.deviceTypes=DEVICE_PRESETS;',stageCtx);
const layout=stageCtx.stageDefault();stageCtx.stageValidate(layout);assert.equal(layout.parts.length,6);assert(stageCtx.deviceTypes.filter(p=>p.sensor).length>=7);assert.equal(stageCtx.stageDefault('blank').parts.length,0);
const invalidLayout=JSON.parse(JSON.stringify(layout));invalidLayout.parts[0].x=Infinity;assert.throws(()=>stageCtx.stageValidate(invalidLayout));invalidLayout.parts[0].x=400;invalidLayout.parts[0].binding='javascript:bad';assert.throws(()=>stageCtx.stageValidate(invalidLayout));invalidLayout.parts[0].binding=0;invalidLayout.lines.push([1,2,3,900]);assert.throws(()=>stageCtx.stageValidate(invalidLayout));
let signalCfg=c();signalCfg.scenes[0].cues=[{output:0,start:100,duration:120}];assert.equal(stageCtx.signal({binding:0},true,100,signalCfg,{},true).on,true);assert.equal(stageCtx.signal({binding:0},true,219,signalCfg,{},true).on,true);assert.equal(stageCtx.signal({binding:0},true,220,signalCfg,{},true).on,false);assert.equal(stageCtx.signal({binding:0},false,0,signalCfg,{outputs:[true]},true).on,true);assert.equal(stageCtx.signal({binding:0},false,0,signalCfg,{outputs:[true]},false).unknown,true);assert.equal(stageCtx.signal({binding:'trigger'},false,0,signalCfg,{trigger:true},true).on,true);assert.equal(stageCtx.signal({binding:'servo'},false,0,signalCfg,{servoActive:true,servoAngle:125},true).angle,125);console.log('PASS: layout validation, device categories, short-pulse boundaries, live output/trigger/servo and stale status');

assert.equal(stageCtx.signal({binding:'servo'},false,0,{outputs:[]},{},true).angle,90);assert.equal(stageCtx.deviceTypes.filter(p=>p.id.startsWith('audio')).length,10);

let dyCfg=context.check(c());dyCfg.audio.player='dy';dyCfg.audio.enabled=true;context.check(dyCfg);dyCfg.audio.player='unknown';assert.throws(()=>context.check(dyCfg));assert.equal(context.check(c()).audio.player,'dfplayer');console.log('PASS: serial player selection and legacy config migration');
vm.runInContext('this.motionPreset=servoPresetMoves;this.applyMotion=applyServoPreset;',context);
const motion={kind:'sweep',start:1000,a:60,b:120,out:1000,back:500,holdA:200,holdB:300,repeats:2};
const limits={minAngle:20,maxAngle:160};let plan=context.motionPreset(motion,limits);
assert.equal(plan.moves.length,5);assert.equal(plan.moves[1].start,1700);assert.equal(plan.moves[2].start,3000);assert.equal(plan.end,5700);
assert.equal(context.servoAngle({servoMoves:plan.moves},2900,90),120);
for(const kind of ['sweep','hold','jerk','tremble','peek'])assert(context.motionPreset({...motion,kind},limits).moves.length>0);
assert.throws(()=>context.motionPreset({...motion,a:10},limits));assert.throws(()=>context.motionPreset({...motion,out:0},limits));assert.throws(()=>context.motionPreset({...motion,repeats:16},limits));assert.throws(()=>context.motionPreset({...motion,start:119000},limits));
let pc=context.check(c());pc.scenes[0].servoMoves=[];let result=context.applyMotion(pc,0,motion,false,true);assert.equal(result.scenes[0].servoMoves.length,5);assert.equal(pc.scenes[0].servoMoves.length,0);assert.throws(()=>context.applyMotion(result,0,motion,false,true));assert.equal(context.applyMotion(result,0,motion,true,true).scenes[0].servoMoves.length,5);
assert.throws(()=>context.applyMotion(pc,0,{...motion,start:9000},false,false));assert.equal(context.applyMotion(pc,0,{...motion,start:9000},false,true).scenes[0].duration,13700);
console.log('PASS: five servo presets, held angles, timing, travel limits, repeats, overlap rejection, append/replace, scene extension');


let light=context.check(c());assert.equal(light.storm.pin,14);assert.equal(light.storm2.pin,33);light.storm.enabled=light.storm2.enabled=true;context.check(light);light.storm2.pin=14;assert.throws(()=>context.check(light));light.storm2.pin=33;light.storm.pin=25;assert.throws(()=>context.check(light));light.storm.pin=14;light.storm.event.effect=8;assert.throws(()=>context.check(light));light.storm.addressable=true;context.check(light);light.scenes[0].lightCues=[{channel:1,start:7000,duration:2400}];context.check(light);light.scenes[0].lightCues.push({channel:1,start:8000,duration:500});assert.throws(()=>context.check(light));light.scenes[0].lightCues.pop();light.scenes[0].lightCues[0].duration=4000;assert.throws(()=>context.check(light));let old=context.check(c());delete old.storm.pin;delete old.storm.idle;delete old.storm.event;old.storm.enabled=true;assert.equal(context.check(old).storm.enabled,false);vm.runInContext('this.lightPreview=previewLightColor;',context);let profile={effect:7,brightness:100,speed:1000,width:1,color:0xffffff,secondary:0};assert(context.lightPreview({...light.storm,pixels:10},profile,0,0,0));assert.equal(context.lightPreview({...light.storm,pixels:10},profile,0,0,1),0);assert.equal(context.preset(c()).scenes[0].lightCues[0].start,7000);console.log('PASS: nine-channel pins, collisions, white/RGB effect guards, event timing/overlap, safe migration and chase preview');

let pair=context.check(c());assert.equal(pair.audio2.enabled,false);pair.audio.enabled=pair.audio2.enabled=true;pair.servo.enabled=pair.storm.enabled=pair.storm2.enabled=true;pair.audio2.player='dy';context.check(pair);for(const pin of [21,22,25,32,13,14,33]){const x=JSON.parse(JSON.stringify(pair));x.audio2.tx=pin;assert.throws(()=>context.check(x));}pair.audio2.rx=4;assert.throws(()=>context.check(pair));assert(html.includes('stageDeviceControls'));console.log('PASS: optional second-player migration, mixed modules, full nine-channel coexistence and UART conflict guards');

vm.runInContext('this.pneumatic=applyPneumaticPreset;',context);
let air=c();air.scenes[0].cues=[];air.motion={a:-1,b:-1,deadTime:200,kind:'motor'};
const ap={kind:'double',a:0,b:1,start:0,pulse:200,hold:1000,rest:1000,cycles:3,replace:false,extend:true};
air.outputs[0].activeHigh=air.outputs[1].activeHigh=false;
let valve=context.pneumatic(air,ap);assert.equal(valve.scenes[0].cues.length,6);assert.equal(valve.motion.kind,'pneumatic');assert.equal(valve.scenes[0].cues[1].start,1200);assert.equal(air.scenes[0].cues.length,0);
assert.throws(()=>context.pneumatic(valve,ap));assert.equal(context.pneumatic(valve,{...ap,replace:true}).scenes[0].cues.length,6);
assert.throws(()=>context.pneumatic(air,{...ap,hold:100}));assert.throws(()=>context.pneumatic(air,{...ap,b:0}));
let motor=c();motor.motion={a:0,b:1,deadTime:200,kind:'motor'};assert.throws(()=>context.pneumatic(motor,ap));
air.outputs[0].maxOnMs=100;assert.throws(()=>context.pneumatic(air,ap));air.outputs[0].maxOnMs=0;
assert.equal(context.pneumatic(air,{...ap,kind:'single'}).scenes[0].cues.length,3);
assert.throws(()=>context.pneumatic(air,{...ap,start:119000}));
console.log('PASS: pneumatic pulses, polarity, interlock ownership, dead time, maximum ON, conflicts, replace and bounds');

const puker=context.preset(c()),ps=puker.scenes[0],pump=ps.cues.find(q=>q.output===3);
assert.equal(pump.start,7000);assert.equal(pump.duration,15000);assert.equal(ps.duration,26000);assert.equal(ps.lightCues[0].duration,15000);assert.equal(puker.outputs[3].maxOnMs,16000);
assert.equal(ps.cues.filter(q=>q.output===0&&q.start>=7000&&q.start<22000).length,10);assert(ps.cues.length<=32);assert(ps.cues.filter(q=>q.output===0).every(q=>q.duration<=200));
console.log('PASS: 15-second pump/light window, ten bounded retch pulses, recovery and scene limits');
vm.runInContext('this.capture=captureCueGroup;this.insert=insertCueGroup;',context);
let life=context.check(c());life.scenes.push({...JSON.parse(JSON.stringify(life.scenes[0])),name:'Idle life'});life.behavior={random:true,idle:1,idleGap:1000};context.check(life);life.behavior.idle=0;assert.throws(()=>context.check(life));life.behavior.idle=1;
life.scenes[0].audioCues=[{start:0,player:1,action:0,value:2},{start:500,player:1,action:1,value:20}];context.check(life);life.scenes[0].audioCues[1].start=499;assert.throws(()=>context.check(life));life.scenes[0].audioCues[1].start=500;life.scenes[0].audioCues[1].value=31;assert.throws(()=>context.check(life));life.scenes[0].audioCues[1].value=20;
let gc=context.check(c());gc.scenes[0]={name:'Test group',duration:10000,scareTrack:0,cues:[{output:0,start:100,duration:200}],servoMoves:[{start:500,duration:500,angle:120,ease:true}],lightCues:[{channel:1,start:1500,duration:200}],audioCues:[{start:2000,player:0,action:0,value:2}]};let group=context.capture(gc.scenes[0],'Retch and jaw',0,3000);assert.equal(group.audioCues.length,1);assert.throws(()=>context.capture(gc.scenes[0],'cut',150,3000));assert.throws(()=>context.insert(gc,group,0,1));let inserted=context.insert(gc,group,5000,1);assert.equal(inserted.scenes[0].cues[1].start,5100);assert.equal(inserted.scenes[0].servoMoves[1].ease,true);assert.equal(gc.scenes[0].cues.length,1);assert.throws(()=>context.insert(gc,group,119000,1));gc.outputs[0].maxOnMs=200;assert.throws(()=>context.insert(gc,group,5000,2));
assert.equal(context.servoAngle({servoMoves:[{start:0,duration:1000,angle:150,ease:true}]},250,50),66);
console.log('PASS: behavior migration/selection, timed audio validation, group capture/conflicts/scaling/immutability and smooth easing');
vm.runInContext('this.profileValidate=validateDeviceProfile;this.profileDefault=profileDefaults;this.profileApply=applyProfileSettings;this.libValidate=validateInventory;this.propMake=makePropPackage;this.propValidate=validatePropPackage;this.blankProp=freshPropConfig;this.types=PROFILE_TYPES;',context);
for(const [type]of context.types){const prof={schema:1,id:'dev-'+type,name:'Test '+type,model:'',voltage:'',notes:'',type,settings:context.profileDefault(type)};context.profileValidate(prof);}
let invCfg=context.check(c());const servoP={schema:1,id:'dev-servo1',name:'Jaw servo',model:'SG90',voltage:'5V',notes:'Separate supply',type:'servo',settings:context.profileDefault('servo')};servoP.settings.home=110;context.profileValidate(servoP);let applied=context.profileApply(invCfg,'servo',servoP);assert.equal(applied.servo.home,110);assert.equal(applied.servo.pin,invCfg.servo.pin);assert.equal(applied.servo.enabled,invCfg.servo.enabled);assert.equal(invCfg.servo.home,90);assert.throws(()=>context.profileApply(invCfg,'output:0',servoP));
let wrongP=JSON.parse(JSON.stringify(servoP));wrongP.settings.pin=25;assert.throws(()=>context.profileValidate(wrongP));wrongP=JSON.parse(JSON.stringify(servoP));wrongP.settings.minAngle=170;assert.throws(()=>context.profileValidate(wrongP));
const lib={schema:1,activeProp:'Skeleton',devices:[servoP],assignments:{servo:{type:'servo',profileId:servoP.id}}};context.libValidate(lib);const complete=context.propMake('Skeleton',invCfg,stageCtx.stageDefault(),lib);context.propValidate(complete);assert.equal(complete.inventory.devices.length,1);assert.equal(complete.config.servo.pin,13);let unassigned=JSON.parse(JSON.stringify(lib));unassigned.assignments={};assert.equal(context.propMake('Blank',invCfg,stageCtx.stageDefault(),unassigned).inventory.devices.length,0);const freshProp=context.blankProp();assert.equal(freshProp.scenes[0].cues.length,0);assert.equal(freshProp.servo.enabled,false);assert.equal(freshProp.audio2.enabled,false);assert.equal(freshProp.behavior.idle,-1);assert.equal(freshProp.outputs.length,6);
let badLib=JSON.parse(JSON.stringify(lib));badLib.devices.push(servoP);assert.throws(()=>context.libValidate(badLib));badLib=JSON.parse(JSON.stringify(lib));badLib.assignments['output:0']={type:'pir',profileId:''};assert.throws(()=>context.libValidate(badLib));
console.log('PASS: all hardware profile presets, travel validation, GPIO/enable preservation, category guards, inventory IDs/assignments, complete prop roundtrip and blank reset');

vm.runInContext('this.outer=outerRowPreset;this.outerOrder=preferOuter;',context);
const outer=context.outer(c());assert.deepEqual(Array.from(outer.outputs,o=>o.pin),[25,27,33,14,18,19]);assert.equal(outer.servo.pin,4);assert.equal(outer.trigger.pin,32);assert.equal(outer.audio2.tx,13);assert.equal(outer.storm.pin,23);assert.equal(outer.storm2.pin,26);const full=JSON.parse(JSON.stringify(outer));for(const k of ['audio','audio2','servo','storm','storm2'])full[k].enabled=true;context.check(full);assert.equal(context.outerOrder([18,25,19,27])[0],25);console.log('PASS: outer-row preset, all-channel pin coexistence and priority ordering');

const sk=context.preset(c());assert(sk.servo.enabled&&sk.storm.enabled&&sk.storm2.enabled);assert(!sk.storm.addressable&&sk.storm2.addressable);assert.equal(sk.behavior.idle,1);assert(sk.scenes[1].randomServo);assert.equal(context.servoAngle(sk.scenes[0],15000,sk.servo.home),sk.servo.home);assert.notEqual(context.servoAngle(sk.scenes[1],2000,90,1),context.servoAngle(sk.scenes[1],2000,90,77));console.log('PASS: skeleton lighting, ambient random head and centered puke head');

const mini=context.boards.find(b=>b.id==='c3-supermini');assert(mini&&!mini.supported);assert.equal(mini.left.length,8);assert.equal(mini.right.length,8);assert.equal(context.positions(mini,0)[0].row,8);assert.equal(context.positions(mini,21)[0].row,8);assert(context.boardSvg(mini,[],false).includes('USB-C END AT TOP'));console.log('PASS: C3 SuperMini reference pinout and separate orientation');


vm.runInContext('this.catalog=BOARD_CATALOG;this.configure=configureBoard;this.fresh=freshPropConfig;this.adapt=adaptPropToBoard;',context);
for(const family of ['esp32c3','esp32s2','esp32s3','esp32c6','esp32']){context.configure(context.catalog[family]);const c=context.fresh();assert.equal(c.outputs.length,context.catalog[family].maxOutputs);for(const k of ['audio','servo','storm','storm2'])c[k].enabled=true;if(context.catalog[family].audioPlayers===2)c.audio2.enabled=true;context.check(c);if(context.catalog[family].audioPlayers===1){c.audio2.enabled=true;assert.throws(()=>context.check(c));c.audio2.enabled=false;}const sk=context.preset(c);context.check(sk);assert.equal(sk.outputs.length,context.catalog[family].maxOutputs);}
console.log('PASS: five family profiles, all-channel collisions, one-player guards and skeleton compatibility');

context.configure(context.catalog.esp32);let portability=context.propMake('Portable',context.fresh(),{schema:1,parts:[],lines:[]},{schema:1,devices:[],assignments:{},activeProp:'Portable'});
context.configure(context.catalog.esp32c3);let reduced=context.adapt(portability);assert.equal(reduced.config.outputs.length,4);context.check(reduced.config);context.configure(context.catalog.esp32);console.log('PASS: cross-family prop adaptation keeps a reviewable valid draft');


context.configure(context.catalog.esp32);portability.config.outputs[0].activeHigh=false;portability.config.trigger.activeHigh=true;portability.config.trigger.pull=0;context.configure(context.catalog.esp32c3);const activeLowImport=context.adapt(portability);assert.equal(activeLowImport.config.outputs[0].activeHigh,false);assert.equal(activeLowImport.config.trigger.activeHigh,true);assert.equal(activeLowImport.config.trigger.pin,10);context.configure(context.catalog.esp32);console.log('PASS: adaptation preserves driver polarity and sensor activation');
