#include "../firmware/HauntCue/AudioProtocol.h"
#include <assert.h>
#include <stdio.h>
using namespace haunt;
int main(){uint8_t p[10];assert(audioPacket(true,0x16,0,p)==4&&p[0]==0xaa&&p[1]==4&&p[3]==0xae);assert(audioPacket(true,0x12,258,p)==6&&p[3]==1&&p[4]==2&&p[5]==0xb6);assert(audioPacket(true,0x06,20,p)==5&&p[1]==0x13&&p[3]==20&&p[4]==0xd2);assert(audioPacket(true,0x19,0,p)==5&&p[1]==0x18&&p[3]==1&&p[4]==0xc4);assert(audioPacket(true,0x19,1,p)==5&&p[3]==2&&p[4]==0xc5);assert(audioPacket(true,0x09,2,p)==5&&p[1]==0x0b&&p[3]==2&&p[4]==0xb8);assert(audioPacket(false,0x12,2,p)==10&&p[0]==0x7e&&p[7]==0xfe&&p[8]==0xe7&&p[9]==0xef);puts("PASS: actual DY/DFPlayer frames, big-endian track, checksum, volume, stop, flash selection and loop modes");}
