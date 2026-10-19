#pragma once
#include <stdint.h>
namespace haunt {
// NC contact: LOW is healthy, HIGH/open is a stop. Stops are immediate;
// only releasing/resetting requires a stable healthy input.
class PhysicalStop {
public:
  bool enabled=false,latched=false,open=false;
  void configure(bool active,uint32_t now){enabled=active;latched=active;open=active;healthy=false;healthyAt=now;}
  void sample(bool contactOpen,uint32_t now){
    if(!enabled){open=false;latched=false;healthy=false;return;}
    open=contactOpen;
    if(open){latched=true;healthy=false;}
    else if(!healthy){healthy=true;healthyAt=now;}
  }
  bool ready(uint32_t now)const{return enabled&&!open&&healthy&&uint32_t(now-healthyAt)>=100;}
  bool reset(uint32_t now){if(!ready(now))return false;latched=false;return true;}
  bool blocked()const{return enabled&&(latched||open);}
private:
  bool healthy=false;uint32_t healthyAt=0;
};
inline bool physicalStopSelfTest(){
  PhysicalStop s;s.configure(false,0);s.sample(true,1);if(s.blocked())return false;
  s.configure(true,0);if(!s.blocked()||s.reset(1000))return false;
  s.sample(false,10);if(s.reset(109)||!s.reset(110)||s.blocked())return false;
  s.sample(true,111);if(!s.blocked()||s.reset(500))return false;
  s.sample(false,120);s.sample(true,121);s.sample(false,130);
  if(s.reset(229)||!s.reset(230))return false;
  s.sample(true,231);s.sample(false,0xfffffff0U);
  if(s.reset(0x40U)||!s.reset(0x54U))return false;
  s.configure(true,300);return s.blocked();
}
}
