#pragma once
#include <stdint.h>
namespace haunt {
// Emit canonical stop/track/loop commands to an independent player slot.
template<class Emit> void routeAmbient(bool first,bool second,uint16_t track,bool resume,Emit emit){
 if(second){emit(1,0x19,1);emit(1,0x16,0);}
 if(first&&!(second&&resume)){emit(0,0x19,1);emit(0,0x16,0);if(track){emit(0,0x12,track);emit(0,0x19,0);}}
}
template<class Emit> void routeScare(bool first,bool second,uint16_t track,Emit emit){
 if(!first&&!second)return;
 unsigned k=second?1:0;emit(k,0x19,1);emit(k,0x16,0);if(track)emit(k,0x12,track);
}
}
