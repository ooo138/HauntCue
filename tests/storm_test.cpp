#include "../firmware/HauntCue/StormEngine.h"
#include <cassert>
#include <cstdio>
using namespace haunt;
int main(){StormConfig c;c.enabled=true;c.bursts=3;c.minWait=c.maxWait=500;c.flashMs=60;c.gapMs=90;StormEngine e;
assert(e.tick(c,true,100,2));assert(e.tick(c,true,159,0));assert(!e.tick(c,true,160,0));assert(!e.tick(c,true,249,0));assert(e.tick(c,true,250,0));assert(!e.tick(c,true,310,0));assert(e.tick(c,true,400,0));assert(!e.tick(c,true,460,0));assert(!e.tick(c,true,959,0));assert(e.tick(c,true,960,0));assert(!e.tick(c,false,970,0));assert(!e.active);assert(e.tick(c,true,980,0));
e.reset();assert(e.tick(c,true,0xfffffff0,0));assert(!e.tick(c,true,44,0));assert(!e.tick(c,false,45,0));
assert(!validateStorm(c,6,-1,-1,true));assert(validateStorm(c,6,5,2,true));assert(validateStorm(c,6,-1,-1,false));c.pixels=61;assert(validateStorm(c,6,-1,-1,true));c.pixels=60;c.maxWait=499;assert(validateStorm(c,6,-1,-1,true));
assert(stormByte(0x123456,0,100)==0x34);assert(stormByte(0x123456,1,100)==0x12);assert(stormByte(0x123456,2,100)==0x56);assert(stormByte(0xffffff,0,50)==127);assert(stormByte(0xffffff,0,0)==0);
puts("PASS: lightning burst boundaries, random count, quiet delays, disarm reset, rollover, resource guards and GRB brightness bytes");}
