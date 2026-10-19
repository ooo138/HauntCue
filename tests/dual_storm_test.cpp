#include "../firmware/HauntCue/CueEngine.h"
#include <cassert>
#include <cstdio>
using namespace haunt;
int main(){Config c;int pins[]={25,26,27,18,19,23};for(int i=0;i<6;i++)c.outputs[i].pin=pins[i];c.storm.enabled=c.storm2.enabled=true;assert(!validate(c));c.storm2.pin=14;assert(validate(c));c.storm2.pin=33;StormEngine a,b;assert(a.tick(c.storm,true,100,0));assert(!b.tick(c.storm2,true,100,0,250));assert(!b.tick(c.storm2,true,349,0,250));assert(b.tick(c.storm2,true,350,0,250));assert(!a.tick(c.storm,false,351,0));assert(!b.tick(c.storm2,false,351,0));puts("PASS: dedicated pins, independent startup/timing and both locations stop");}
