#include "../firmware/HauntCue/CueEngine.h"
#include <assert.h>
#include <stdio.h>
using namespace haunt;
Config sample() {
  Config c; c.outputCount=2; c.outputs[0].pin=25; c.outputs[1].pin=26;
  c.scenes[0].duration=1000; c.scenes[0].count=3;
  c.scenes[0].cues[0]={0,0,600}; c.scenes[0].cues[1]={0,400,500}; c.scenes[0].cues[2]={1,100,200};
  c.cooldown=200; c.debounce=40; return c;
}
int main() {
  // Fixed timer works without a physical sensor and stops on disarm.
  {
    Config c=sample(); c.triggerEnabled=false; c.repeat=true;
    c.repeatMin=c.repeatMax=1000; Engine e;
    e.configure(&c,0,true);e.arm(true,0,true);e.tick(0,true);
    e.tick(999,true);assert(!e.running);e.tick(1000,true);assert(e.running);
    e.tick(2000,true);assert(!e.running);e.tick(2999,true);assert(!e.running);
    e.tick(3000,true);assert(e.running);e.arm(false,3100,true);
    e.tick(10000,true);assert(!e.running);
  }
  // A disabled sensor cannot start a scene; manual Fire still works.
  {
    Config c=sample(); c.triggerEnabled=false; Engine e;
    e.configure(&c,0,true); e.arm(true,0,true);
    e.tick(100,false); e.tick(150,false);
    assert(!e.running); assert(!e.triggerActive());
    assert(e.start(200));
    c.triggerEnabled=true; e.configure(&c,0,true); e.arm(true,0,true);
    e.tick(100,false); e.tick(150,false); assert(e.running);
  }
  {
    Config sv=sample();sv.servo=true;sv.servoPin=13;Scene& scene=sv.scenes[0];
    scene.servoCount=2;scene.servoMoves[0]={100,200,150};scene.servoMoves[1]={500,200,30};
    assert(!validate(sv));assert(servoAngle(scene,0,90)==90);assert(servoAngle(scene,100,90)==90);
    assert(servoAngle(scene,200,90)==120);assert(servoAngle(scene,300,90)==150);
    assert(servoAngle(scene,500,90)==150);assert(servoAngle(scene,600,90)==90);assert(servoAngle(scene,900,90)==30);
    sv.servoPin=25;assert(validate(sv));sv.servoPin=13;scene.servoMoves[1].start=200;assert(validate(sv));
    scene.servoMoves[1].start=500;scene.servoMoves[1].angle=180;assert(validate(sv));
  }
  {
    Config r=sample();r.repeat=true;r.repeatMin=1000;r.repeatMax=2000;
    Engine autoEngine;autoEngine.configure(&r,0,true);autoEngine.arm(true,0,true);
    autoEngine.tick(0,true,500);assert(autoEngine.repeatRemaining(0)==1500);
    autoEngine.tick(1499,true);assert(!autoEngine.running);autoEngine.tick(1500,true);assert(autoEngine.running);
    autoEngine.tick(2500,true,0);assert(!autoEngine.running);assert(autoEngine.repeatRemaining(2500)==1000);
    autoEngine.arm(false,2600,true);autoEngine.tick(10000,true);assert(!autoEngine.running);
    r.outputs[0].maxOnMs=200;r.outputs[1].maxOnMs=200;r.interlockA=0;r.interlockB=1;
    OutputGuard g;bool wanted[MAX_OUTPUTS]={true,false};g.tick(r,wanted,0);assert(g.state[0]);
    g.tick(r,wanted,200);assert(!g.state[0]);g.tick(r,wanted,900);assert(!g.state[0]);
    wanted[0]=false;g.tick(r,wanted,901);wanted[1]=true;g.tick(r,wanted,902);assert(g.state[1]);
    wanted[0]=true;g.tick(r,wanted,910);assert(!g.state[0]&&!g.state[1]);
    wanted[1]=false;g.tick(r,wanted,1000);assert(!g.state[0]);g.tick(r,wanted,1110);assert(g.state[0]);
    g.reset();wanted[0]=true;wanted[1]=true;g.tick(r,wanted,2000);assert(!g.state[0]&&!g.state[1]);
    // A 500ms manual test is capped by the same hardware guard.
    Engine testEngine;testEngine.configure(&r,0,true);assert(testEngine.test(0,0));
    g.reset();wanted[0]=testEngine.on(0,0);wanted[1]=false;g.tick(r,wanted,0);assert(g.state[0]);
    wanted[0]=testEngine.on(0,201);g.tick(r,wanted,201);assert(!g.state[0]);
  }
  Config c=sample(); assert(!validate(c));
  Engine e; e.configure(&c,0,true); assert(!e.start(0)); assert(!e.on(0,0));
  e.arm(true,0,true); assert(e.start(0)); assert(e.on(0,0)); assert(!e.on(1,99));
  assert(e.on(1,100)); assert(!e.on(1,300)); assert(e.on(0,600)); assert(e.on(0,899)); assert(!e.on(0,900));
  e.tick(1000,true); assert(!e.running); assert(!e.on(0,1000)); assert(!e.start(1100)); assert(e.start(1200));
  e.stop(1250); assert(!e.on(0,1250)); e.arm(false,1250,true); assert(!e.start(2000));
  assert(e.test(1,2000)); assert(e.on(1,2499)); assert(!e.on(1,2500)); e.tick(2500,true); assert(e.test(0,2501));
  e.arm(true,2600,true); assert(!e.test(0,2600));
  // A brief bounce must not fire; a stable press fires exactly once.
  e.configure(&c,0,true); e.arm(true,0,true);
  e.tick(10,false); e.tick(20,true); e.tick(70,true); assert(!e.running);
  e.tick(100,false); e.tick(139,false); assert(!e.running); e.tick(140,false); assert(e.running && e.runCount==3);
  e.tick(1140,false); e.tick(1400,false); assert(!e.running); // held trigger cannot repeat
  e.tick(1500,true); e.tick(1540,true); e.tick(1600,false); e.tick(1640,false); assert(e.running);
  // Presses during cooldown are dropped, not queued.
  e.stop(1650); e.tick(1660,true); e.tick(1700,true); e.tick(1710,false); e.tick(1750,false); assert(!e.running);
  e.tick(2000,false); assert(!e.running); e.tick(2010,true); e.tick(2050,true); e.tick(2060,false); e.tick(2100,false); assert(e.running);
  // Arming while held requires release before a new edge.
  e.configure(&c,0,false); e.arm(true,0,false); e.tick(50,false); assert(!e.running);
  e.tick(100,true); e.tick(140,true); e.tick(200,false); e.tick(240,false); assert(e.running);
  // millis rollover preserves cue timing and cooldown.
  e.configure(&c,0xFFFFFF00u,true); e.arm(true,0xFFFFFF00u,true); assert(e.start(0xFFFFFF00u));
  assert(e.on(0,uint32_t(0xFFFFFF00u+600u))); e.tick(uint32_t(0xFFFFFF00u+1000u),true); assert(!e.running);
  assert(e.cooldownRemaining(uint32_t(0xFFFFFF00u+1100u))==100); assert(e.start(uint32_t(0xFFFFFF00u+1200u)));
  // Independent scenes and malformed hardware/timing are validated.
  c.outputs[1].pin=25; assert(validate(c)); c=sample(); c.outputs[0].pin=6; assert(validate(c));
  c=sample(); c.triggerPin=34; assert(validate(c)); c.pull=0; assert(!validate(c));
  c=sample(); c.audio=true; c.audioTx=25; assert(validate(c)); c.audioTx=21; assert(!validate(c));
  c.scenes[0].cues[0].duration=1001; assert(validate(c)); c=sample(); c.scenes[0].cues[0].output=2; assert(validate(c));
  c=sample(); c.sceneCount=2; c.scenes[1]=c.scenes[0]; c.selected=1; c.scenes[1].count=0;
  e.configure(&c,0,true); e.arm(true,0,true); e.start(0); assert(!e.on(0,0));
  c=sample();c.interlockA=0;c.interlockB=1;c.outputs[0].activeHigh=false;c.outputs[1].activeHigh=false;c.scenes[0].count=0;
  assert(validate(c));c.pneumaticInterlock=true;assert(!validate(c));
  puts("PASS: cue boundaries, overlaps, debounce, held triggers, cooldown, disarm, test deadlines, scene selection, pin validation, millis rollover");
}

