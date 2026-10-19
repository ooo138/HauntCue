#pragma once
namespace haunt {
#if defined(CONFIG_IDF_TARGET_ESP32C3)
constexpr const char* BOARD_FAMILY="esp32c3";constexpr int BOARD_OUTPUTS=4,BOARD_AUDIO=1;
constexpr int BOARD_PINS[]={0,1,3,4},BOARD_TRIGGER=10,BOARD_SERVO=5,BOARD_LIGHT1=6,BOARD_LIGHT2=7,BOARD_TX=21,BOARD_RX=20,BOARD_TX2=3,BOARD_RX2=4;
inline bool boardOutputPin(int p){return p==0||p==1||(p>=3&&p<=7)||p==10||p==20||p==21;}
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
constexpr const char* BOARD_FAMILY="esp32c6";constexpr int BOARD_OUTPUTS=6,BOARD_AUDIO=1;
constexpr int BOARD_PINS[]={0,1,2,3,4,5},BOARD_TRIGGER=6,BOARD_SERVO=7,BOARD_LIGHT1=10,BOARD_LIGHT2=11,BOARD_TX=21,BOARD_RX=20,BOARD_TX2=4,BOARD_RX2=5;
inline bool boardOutputPin(int p){return (p>=0&&p<=7)||p==10||p==11||(p>=18&&p<=23);}
#elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#if defined(CONFIG_IDF_TARGET_ESP32S2)
constexpr const char* BOARD_FAMILY="esp32s2";constexpr int BOARD_AUDIO=1;
#else
constexpr const char* BOARD_FAMILY="esp32s3";constexpr int BOARD_AUDIO=2;
#endif
constexpr int BOARD_OUTPUTS=6;
constexpr int BOARD_PINS[]={4,5,6,7,15,16},BOARD_TRIGGER=17,BOARD_SERVO=18,BOARD_LIGHT1=8,BOARD_LIGHT2=9,BOARD_TX=21,BOARD_RX=14,BOARD_TX2=10,BOARD_RX2=11;
inline bool boardOutputPin(int p){return p>=4&&p<=18||p==21;}
#else
constexpr const char* BOARD_FAMILY="esp32";constexpr int BOARD_OUTPUTS=6,BOARD_AUDIO=2;
constexpr int BOARD_PINS[]={25,26,27,18,19,23},BOARD_TRIGGER=32,BOARD_SERVO=13,BOARD_LIGHT1=14,BOARD_LIGHT2=33,BOARD_TX=21,BOARD_RX=22,BOARD_TX2=4,BOARD_RX2=34;
inline bool boardOutputPin(int p){return p==4||p==13||p==14||p==18||p==19||p==21||p==22||p==23||p==25||p==26||p==27||p==32||p==33;}
#endif
inline bool boardInputPin(int p){return boardOutputPin(p)||(BOARD_FAMILY[5]==0&&(p==16||p==34||p==35||p==36||p==39));}
inline bool boardNeedsExternalPull(int p){return BOARD_FAMILY[5]==0&&p>=34;}
}
