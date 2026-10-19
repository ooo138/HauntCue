#pragma once
#include <stdint.h>
#include <stddef.h>
namespace haunt {
// Control frames only; file loading uses the player's own USB interface.
inline size_t audioPacket(bool dy,uint8_t cmd,uint16_t value,uint8_t* packet){
  if(dy){uint8_t code=0,len=0;
    switch(cmd){case 0x09:code=0x0b;len=1;value=2;break;case 0x06:code=0x13;len=1;break;case 0x16:code=0x04;break;case 0x19:code=0x18;len=1;value=value?2:1;break;case 0x12:code=0x07;len=2;break;default:return 0;}
    packet[0]=0xaa;packet[1]=code;packet[2]=len;
    if(len==1){packet[3]=uint8_t(value);}
    if(len==2){packet[3]=uint8_t(value>>8);packet[4]=uint8_t(value);}
    uint8_t sum=0;for(size_t i=0;i<size_t(3+len);++i)sum=uint8_t(sum+packet[i]);packet[3+len]=sum;return 4+len;
  }
  const uint8_t base[]={0x7e,0xff,0x06,cmd,0,uint8_t(value>>8),uint8_t(value),0,0,0xef};for(size_t i=0;i<10;++i)packet[i]=base[i];uint16_t sum=0;for(size_t i=1;i<=6;++i)sum+=packet[i];const uint16_t check=uint16_t(0-sum);packet[7]=uint8_t(check>>8);packet[8]=uint8_t(check);return 10;
}
}
