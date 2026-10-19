#include <assert.h>
#include <stdio.h>
#include "../firmware/HauntCue/CueEngine.h"
#include "../firmware/HauntCue/ArmInput.h"
using namespace haunt;
int main(){
 Config c;c.triggerEnabled=false;c.outputCount=2;c.outputs[0].pin=25;c.outputs[1].pin=26;c.triggerPin=32;c.sceneCount=2;c.scenes[0].duration=1000;c.scenes[1].duration=2000;
 for(unsigned i=0;i<128;i++)c.scenes[1].cues[i]={uint8_t(i%2),i*10,5};c.scenes[1].count=128;c.scenes[1].enabledMask=2;
 assert(!validate(c));Engine e;e.configure(&c,0,true);assert(e.preview(1,100));assert(!e.armed&&e.running&&e.previewing);assert(c.selected==0);assert(!e.on(0,100));assert(e.on(1,110));assert(!e.on(1,115));assert(e.on(1,1370));e.tick(2100,true);assert(!e.running&&!e.previewing&&!e.armed);assert(!e.on(0,2100)&&!e.on(1,2100));
 assert(e.test(0,2200));assert(e.on(0,2200));e.tick(2700,true);assert(!e.on(0,2700));e.arm(true,2800,true);assert(!e.preview(1,2800));e.arm(false,2900,true);c.scenes[1].enabledMask=2047;assert(e.preview(1,3000));assert(e.on(0,3000));e.stop(3001);assert(!e.on(0,3001));
 ArmInput arm;arm.configure(true,true,0,true);assert(!arm.sample(true,100));assert(!arm.sample(false,110));assert(!arm.sample(false,150));assert(!arm.sample(true,160));assert(!arm.sample(true,199));assert(arm.sample(true,200)==1);assert(!arm.sample(true,1000));assert(!arm.sample(false,1001));assert(!arm.sample(false,1041));assert(!arm.sample(true,1050));assert(arm.sample(true,1090)==1);
 arm.configure(true,false,0,false);assert(!arm.sample(false,40));assert(!arm.sample(true,50));assert(arm.sample(true,90)==1);assert(!arm.sample(false,100));assert(arm.sample(false,140)==-1);arm.configure(true,true,0xffffffe0U,false);assert(!arm.sample(false,0x10));assert(!arm.sample(true,0x20));assert(arm.sample(true,0x48)==1);
 assert(sizeof(Config)<14000);puts("PASS: 128-cue execution, scene selection, checkbox masks, bounded disarmed sequence tests, manual pulse independence, stop behavior, physical arm debounce, held-boot inhibit and millis rollover");
}
