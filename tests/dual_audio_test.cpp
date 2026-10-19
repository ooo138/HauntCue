#include "../firmware/HauntCue/CueEngine.h"
#include "../firmware/HauntCue/AudioRouting.h"
#include "../firmware/HauntCue/AudioProtocol.h"
#include <vector>
#include <cassert>
#include <cstdio>
using namespace haunt;
struct Cmd{unsigned slot,cmd,value;};
int main(){std::vector<Cmd> v;auto emit=[&](unsigned k,uint8_t cmd,uint16_t value){v.push_back({k,cmd,value});};
 muteScenePlayers(2047,emit);assert(v.empty());muteScenePlayers(2047^(1u<<9),emit);assert(v.size()==2);for(auto c:v)assert(c.slot==0);v.clear();muteScenePlayers(2047^(1u<<10),emit);assert(v.size()==2);for(auto c:v)assert(c.slot==1);v.clear();muteScenePlayers(0,emit);assert(v.size()==4);v.clear();
 routeAmbient(true,true,1,false,emit);assert(v.size()==6);v.clear();routeScare(true,true,2,emit);assert(v.size()==3);for(auto c:v)assert(c.slot==1);v.clear();routeAmbient(true,true,1,true,emit);assert(v.size()==2);for(auto c:v)assert(c.slot==1);v.clear();routeScare(true,false,2,emit);assert(v.size()==3);for(auto c:v)assert(c.slot==0);v.clear();routeAmbient(true,false,1,true,emit);assert(v.size()==4);v.clear();routeScare(false,false,2,emit);assert(v.empty());routeScare(false,true,0,emit);assert(v.size()==2);
 uint8_t packet[10];assert(audioPacket(false,0x12,2,packet)==10&&packet[0]==0x7e&&packet[6]==2);assert(audioPacket(true,0x12,2,packet)==6&&packet[0]==0xaa&&packet[1]==7&&packet[4]==2);assert(audioPacket(true,0x19,0,packet)==5&&packet[3]==1);
 Config c;int pins[]={25,26,27,18,19,23};for(int i=0;i<6;i++)c.outputs[i].pin=pins[i];c.audio=c.audio2=c.servo=c.storm.enabled=c.storm2.enabled=true;assert(!validate(c));for(int pin:{21,22,25,32,13,14,33}){c.audio2Tx=pin;assert(validate(c));}c.audio2Tx=4;c.audio2Rx=21;assert(validate(c));c.audio2Rx=34;c.audio2DY=true;assert(!validate(c));puts("PASS: dual routing leaves ambient untouched, single-player fallback, silence, DF/DY packets and full GPIO coexistence");}
