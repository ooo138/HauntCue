#pragma once
#include <stdint.h>
namespace haunt {
struct LightProfile { uint8_t effect=2,brightness=70,width=4;uint16_t speed=1500;uint32_t color=0xffffff,secondary=0x552200; };
struct LightCue {uint8_t channel=0;uint32_t start=0,duration=1000;};
inline bool validProfile(const LightProfile& p,bool rgb){return p.effect<=10&&p.brightness<=100&&p.speed>=100&&p.speed<=10000&&p.width>=1&&p.width<=20&&p.color<=0xffffff&&p.secondary<=0xffffff&&(rgb||(p.effect!=7&&p.effect!=8&&p.effect!=10));}
inline uint32_t lightHash(uint32_t x){x^=x>>16;x*=0x7feb352d;x^=x>>15;x*=0x846ca68b;return x^(x>>16);}
inline uint32_t lightScale(uint32_t c,unsigned b){return (((c>>16)&255)*b/100<<16)|(((c>>8)&255)*b/100<<8)|((c&255)*b/100);}
inline uint32_t lightWheel(unsigned x){x%=255;if(x<85)return ((255-x*3)<<16)|(x*3<<8);if(x<170){x-=85;return ((255-x*3)<<8)|x*3;}x-=170;return (x*3<<16)|(255-x*3);}
inline uint32_t lightPixel(const LightProfile& p,uint32_t t,unsigned pixel,unsigned count,bool lightning,uint32_t seed){
 unsigned phase=t%p.speed,tri=phase*200/p.speed;tri=tri>100?200-tri:tri;unsigned b=100;uint32_t color=p.color;
 switch(p.effect){case 0:b=0;break;case 1:break;case 2:b=lightning?100:0;break;case 3:b=25+lightHash(t/(p.speed/10+1)+seed)%76;break;case 4:b=5+tri*95/100;break;case 5:b=(phase<p.speed/8||(phase>p.speed/4&&phase<p.speed*3/8))?100:5;break;case 6:b=phase<p.speed/2?100:0;break;case 7:b=((pixel+count-(t*count/p.speed)%count)%count)<p.width?100:0;break;case 8:color=lightWheel(pixel*255/count+t*255/p.speed);break;case 9:b=20+lightHash(t/(p.speed/12+1)+seed+pixel*17)%81;color=((pixel+t/(p.speed/10+1))%7<2)?p.secondary:p.color;break;case 10:b=(lightHash(t/p.speed+pixel*7919+seed)%7==0)?100-tri:0;break;default:b=0;}
 return lightScale(color,b*p.brightness/100);
}
}
