#pragma once
#include <stdint.h>
#include "LightingEffects.h"
namespace haunt {
struct StormConfig { bool enabled=false, addressable=false, ambient=true; int8_t pin=14;LightProfile idle,event = []{LightProfile p;p.effect=5;p.color=0x66ff00;p.secondary=0xd9ff44;p.speed=600;return p;}(); uint8_t channel=5,pixels=30,brightness=70,bursts=4; uint32_t color=0xFFFFFF,minWait=3000,maxWait=12000; uint16_t flashMs=60,gapMs=90; };
inline const char* validateStorm(const StormConfig& s, unsigned count,int a,int b,bool activeHigh) {
 if(s.channel>=6||(s.enabled&&s.channel>=count)||s.pixels<1||s.pixels>60||s.brightness<1||s.brightness>100||s.bursts<1||s.bursts>8||s.color>0xFFFFFF||s.minWait<500||s.maxWait>60000||s.minWait>s.maxWait||s.flashMs<10||s.flashMs>300||s.gapMs<20||s.gapMs>500)return "Invalid lightning settings.";
 if(s.enabled&&(s.channel==a||s.channel==b||!activeHigh))return "Lightning needs an Active HIGH channel outside the motor interlock.";
 return nullptr;
}
struct StormEngine {
 bool active=false,lit=false; uint8_t remaining=0; uint32_t next=0;
 void reset(){active=lit=false;remaining=0;next=0;}
 bool tick(const StormConfig& s,bool allowed,uint32_t now,uint32_t random,uint32_t initialDelay=0) {
  if(!s.enabled||!allowed){reset();return false;}
  if(!active){active=true;next=now+initialDelay;}
  if(int32_t(now-next)>=0){
   if(lit){lit=false;if(--remaining)next=now+s.gapMs;else next=now+s.minWait+random%(s.maxWait-s.minWait+1);}
   else {if(!remaining)remaining=1+random%s.bursts;lit=true;next=now+s.flashMs;}
  }
  return lit;
 }
};
inline uint8_t stormByte(uint32_t color,unsigned byte,uint8_t brightness){unsigned shift=byte==0?8:byte==1?16:0;return uint8_t(((color>>shift)&255)*brightness/100);}
}
