#pragma once
#include <stdint.h>
namespace haunt {
// NO dry contact to ground. A released contact must be observed after boot;
// neither booting with a held button nor a latched stop can arm a controller.
class ArmInput {
public:
  bool enabled=false,toggle=true;
  void configure(bool active,bool momentary,uint32_t now,bool pressed) {
    enabled=active;toggle=momentary;candidate=stable=pressed;released=false;changed=now;
  }
  // 1 = arm/toggle press, -1 = maintained switch released, 0 = no event.
  int sample(bool pressed,uint32_t now) {
    if(!enabled)return 0;
    if(pressed!=candidate){candidate=pressed;changed=now;}
    if(uint32_t(now-changed)<40)return 0;
    if(!candidate)released=true;
    if(candidate==stable)return 0;
    stable=candidate;
    if(!stable)return toggle?0:-1;
    if(!released)return 0;
    released=false;return 1;
  }
  bool active()const{return stable;}
private:
  bool candidate=false,stable=false,released=false;
  uint32_t changed=0;
};
}
