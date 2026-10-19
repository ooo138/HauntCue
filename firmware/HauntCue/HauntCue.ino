#include <Arduino.h>
#include <memory>

#include <esp32-hal-rmt.h>

#include <WiFi.h>

#include <WebServer.h>

#include <ESPmDNS.h>

#include <Preferences.h>

#include <LittleFS.h>

#include <ArduinoJson.h>

#include "CueEngine.h"
#include "PhysicalStop.h"
#include "ArmInput.h"
#include "ConfigValidation.h"

#include "WebUI.h"

#include "AudioProtocol.h"
#include "AudioRouting.h"



#if !defined(CONFIG_IDF_TARGET_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C6)

#error "This chip family has no HauntCue build."

#endif



using namespace haunt;

WebServer server(80);

Preferences prefs;

Config config;
ArmInput armInput;
int armInputPin=-1;
volatile bool otaBusy=false;
bool configLoadFault=false;

Engine engine;

OutputGuard outputGuard;

HardwareSerial player(BOARD_AUDIO==2?2:1), player2(BOARD_AUDIO==2?1:0);

bool servoAttached=false, servoLive=false, servoFault=false;

uint32_t servoLiveAt=0;

uint8_t servoLiveAngle=90, servoCommandedAngle=90;

void disableServo() {

  if (servoAttached) ledcDetach(config.servoPin);

  servoAttached=false; servoLive=false;

  if (config.servo) digitalWrite(config.servoPin,LOW);

}

void servoTick(uint32_t now) {

  if (servoLive && uint32_t(now-servoLiveAt)>=2000) servoLive=false;

  if (!config.servo || ((engine.running||engine.idle)&&!(config.scenes[engine.sceneIndex()].enabledMask&(1u<<6))) || (!engine.armed && !engine.running && !servoLive)) { disableServo(); return; }

  if (!servoAttached) {

    servoAttached=ledcAttach(config.servoPin,50,16);

    if (!servoAttached) { servoFault=true; return; }

  }

  static uint32_t servoGeneration=0,servoSeed=1;if(servoGeneration!=engine.generation()){servoGeneration=engine.generation();servoSeed=esp_random();}
  servoCommandedAngle=servoLive ? servoLiveAngle : (engine.running||engine.idle) ? servoAngle(config.scenes[engine.sceneIndex()],engine.elapsed(now),config.servoHome,servoSeed) : config.servoHome;

  uint32_t pulse=config.servoMinUs+(config.servoMaxUs-config.servoMinUs)*servoCommandedAngle/180;

  if (!ledcWrite(config.servoPin,(pulse*65535UL+10000)/20000)) servoFault=true;

}



String adminPassword, wifiSSID, wifiPassword, apName, deviceName, deviceHost;

bool discoveryReady=false;

bool fsOK = false, nvsOK = false, lastRun = false;

uint32_t audioBootAt = 0, rebootAt = 0;

bool rebootPending = false;

SemaphoreHandle_t engineMutex;

struct EngineLock {

  EngineLock() { xSemaphoreTake(engineMutex, portMAX_DELAY); }

  ~EngineLock() { xSemaphoreGive(engineMutex); }

};

const char* buildVersion = "1.33.0";
PhysicalStop physicalStop;
int physicalStopPin=strcmp(BOARD_FAMILY,"esp32")==0?16:strcmp(BOARD_FAMILY,"esp32c3")==0?3:strcmp(BOARD_FAMILY,"esp32c6")==0?18:12;
bool physicalStopConflict(const Config& c,int pin){
 for(unsigned i=0;i<c.outputCount;i++)if(c.outputs[i].pin==pin)return true;
 return (c.triggerEnabled&&c.triggerPin==pin)||(c.servo&&c.servoPin==pin)||(c.storm.enabled&&c.storm.pin==pin)||(c.storm2.enabled&&c.storm2.pin==pin)||(c.audio&&(c.audioTx==pin||c.audioRx==pin))||(c.audio2&&(c.audio2Tx==pin||c.audio2Rx==pin));
}
void configurePhysicalStopPin(){if(physicalStop.enabled&&!physicalStopConflict(config,physicalStopPin))pinMode(physicalStopPin,boardNeedsExternalPull(physicalStopPin)?INPUT:INPUT_PULLUP);}
void halt();
void samplePhysicalStop(uint32_t now){const bool was=physicalStop.blocked();if(physicalStop.enabled)physicalStop.sample(physicalStopConflict(config,physicalStopPin)||digitalRead(physicalStopPin)==HIGH,now);if(!was&&physicalStop.blocked())halt();}




// DFPlayer commands are spaced without blocking the cue scheduler.

struct AudioCommand { uint8_t command;uint16_t value; };
struct AudioRuntime{AudioCommand queue[12];uint8_t head=0,count=0;bool ready=false,sent=false;uint32_t last=0;};
AudioRuntime audioRuntime[2];
bool audioEnabled(unsigned k){return k?config.audio2:config.audio;}
bool audioDY(unsigned k){return k?config.audio2DY:config.audioDY;}
HardwareSerial& audioPort(unsigned k){return k?player2:player;}
void queueAudio(uint8_t cmd,uint16_t value,unsigned k=0){auto& a=audioRuntime[k];if(!audioEnabled(k)||a.count>=12)return;a.queue[(a.head+a.count)%12]={cmd,value};++a.count;}
void clearAudio(unsigned k=0){audioRuntime[k].head=audioRuntime[k].count=0;}
void sendAudio(uint8_t cmd,uint16_t value,unsigned k=0){uint8_t packet[10];size_t len=audioPacket(audioDY(k),cmd,value,packet);if(len)audioPort(k).write(packet,len);audioRuntime[k].last=millis();audioRuntime[k].sent=true;}
void audioTick(uint32_t now){if(uint32_t(now-audioBootAt)<2000)return;for(unsigned k=0;k<2;k++){if(!audioEnabled(k))continue;auto& a=audioRuntime[k];if(!a.ready){AudioCommand pending[10];uint8_t count=min<uint8_t>(a.count,10);for(unsigned i=0;i<count;i++)pending[i]=a.queue[(a.head+i)%12];clearAudio(k);queueAudio(0x09,2,k);queueAudio(0x06,k?config.volume2:config.volume,k);for(unsigned i=0;i<count;i++)queueAudio(pending[i].command,pending[i].value,k);a.ready=true;}if(a.count&&(!a.sent||uint32_t(now-a.last)>=120)){auto item=a.queue[a.head];a.head=(a.head+1)%12;--a.count;sendAudio(item.command,item.value,k);}}}
uint32_t audioProgram=0;AudioCueClock audioClock;bool lastIdle=false,firstAudioTouched=false;
void stopAudio(){lastIdle=false;firstAudioTouched=false;audioClock.reset();for(unsigned k=0;k<2;k++){clearAudio(k);queueAudio(0x19,1,k);queueAudio(0x16,0,k);}}
void ambient(bool resume=false){if(config.audio2){clearAudio(1);if(!resume)clearAudio(0);}else clearAudio(0);if(!resume||!config.audio2)queueAudio(0x06,config.volume,0);queueAudio(0x06,config.volume2,1);routeAmbient(config.audio,config.audio2,config.ambientTrack,resume,[](unsigned k,uint8_t cmd,uint16_t v){queueAudio(cmd,v,k);});}
void applyAudioMask(){const auto mask=config.scenes[engine.sceneIndex()].enabledMask;for(unsigned k=0;k<2;k++)if(!(mask&(1u<<(9+k))))clearAudio(k);muteScenePlayers(mask,[](unsigned k,uint8_t cmd,uint16_t v){queueAudio(cmd,v,k);});if(config.audio&&!(mask&(1u<<9)))firstAudioTouched=true;}
void scare(){if(lastIdle){clearAudio(0);if(firstAudioTouched)ambient(false);}firstAudioTouched=false;audioProgram=engine.generation();audioClock.reset();clearAudio(config.audio2?1:0);queueAudio(0x06,config.audio2?config.volume2:config.volume,config.audio2?1:0);routeScare(config.audio,config.audio2,(config.scenes[engine.sceneIndex()].enabledMask&(1u<<(config.audio2?10:9)))?config.scenes[engine.sceneIndex()].scareTrack:0,[](unsigned k,uint8_t cmd,uint16_t v){queueAudio(cmd,v,k);});applyAudioMask();}

void cueAudioTick(uint32_t now){if(!engine.running&&!engine.idle)return;if(audioProgram!=engine.generation()){audioProgram=engine.generation();audioClock.reset();applyAudioMask();}audioClock.tick(config.scenes[engine.sceneIndex()],engine.elapsed(now),[](const AudioCue& q){if(!audioEnabled(q.player)||!(config.scenes[engine.sceneIndex()].enabledMask&(1u<<(9+q.player))))return true;if(audioRuntime[q.player].count+(q.action==0?2:1)>12)return false;if(q.player==0&&q.action!=1)firstAudioTouched=true;if(q.action==0){queueAudio(0x19,1,q.player);queueAudio(0x12,q.value,q.player);}else queueAudio(q.action==1?0x06:0x16,q.value,q.player);return true;});}
StormEngine stormEngines[2];
uint32_t lightTestStart[2]={};bool lightTesting[2]={},lightTestEvent[2]={};

struct StatusLEDSettings {bool enabled=false;int pin=strcmp(BOARD_FAMILY,"esp32")==0?17:strcmp(BOARD_FAMILY,"esp32c3")==0?5:strcmp(BOARD_FAMILY,"esp32c6")==0?19:13;uint8_t pixels=3,brightness=15;} statusLED;
bool statusLEDAttached=false,statusLEDFault=false;uint32_t statusLEDLast=0xffffffffU;rmt_data_t statusLEDData[8*24+1];
bool statusPinSupported(int p){return outputPin(p)||(strcmp(BOARD_FAMILY,"esp32")==0&&p==17);}
bool statusLEDConflict(const Config& c,int p){return physicalStopConflict(c,p)||(physicalStop.enabled&&physicalStopPin==p)||(armInput.enabled&&armInputPin==p);}
void releaseStatusLED(){if(statusLEDAttached){uint32_t start=millis();while(!rmtTransmitCompleted(statusLED.pin)&&uint32_t(millis()-start)<10)delay(1);for(unsigned i=0;i<statusLED.pixels*24;i++){statusLEDData[i].level0=1;statusLEDData[i].duration0=4;statusLEDData[i].level1=0;statusLEDData[i].duration1=8;}auto& reset=statusLEDData[statusLED.pixels*24];reset.level0=reset.level1=0;reset.duration0=reset.duration1=3000;rmtWrite(statusLED.pin,statusLEDData,statusLED.pixels*24+1,20);rmtDeinit(statusLED.pin);pinMode(statusLED.pin,INPUT);}statusLEDAttached=false;}
void setupStatusLED(){statusLEDFault=false;statusLEDLast=0xffffffffU;if(!statusLED.enabled)return;if(statusLEDConflict(config,statusLED.pin)){statusLEDFault=true;return;}statusLEDAttached=rmtInit(statusLED.pin,RMT_TX_MODE,RMT_MEM_NUM_BLOCKS_1,10000000);statusLEDFault=!statusLEDAttached;}
bool stormAttached[2]={},stormFaults[2]={};int stormLast[2]={-1,-1};

rmt_data_t stormData[2][60*24+1];

const StormConfig& stormConfig(unsigned k){return k?config.storm2:config.storm;}

uint32_t lightStarted[2]={},lightLastFrame[2]={};bool lightWasArmed[2]={},lightEvent[2]={},lightingDark[2]={true,true};uint32_t lightColors[2][60];

void stormFrame(unsigned k){const auto& st=stormConfig(k);auto* data=stormData[k];unsigned n=0;

 for(unsigned pixel=0;pixel<st.pixels;++pixel)for(unsigned byte=0;byte<3;++byte){uint8_t v=stormByte(lightColors[k][pixel],byte,100);for(int bit=7;bit>=0;--bit){bool one=v&(1<<bit);data[n].level0=1;data[n].duration0=one?8:4;data[n].level1=0;data[n++].duration1=one?4:8;}}

 data[n].level0=0;data[n].duration0=3000;data[n].level1=0;data[n].duration1=3000;

}

void lightOff(unsigned k,bool blocking){const auto& st=stormConfig(k);if(!stormAttached[k])return;for(auto& c:lightColors[k])c=0;if(st.addressable){stormFrame(k);if(blocking)rmtWrite(st.pin,stormData[k],st.pixels*24+1,20);else if(rmtTransmitCompleted(st.pin))rmtWriteAsync(st.pin,stormData[k],st.pixels*24+1);}else ledcWrite(st.pin,0);}

void releaseStorm(){lightTesting[0]=lightTesting[1]=false;for(unsigned k=0;k<2;k++){const auto& st=stormConfig(k);if(stormAttached[k]){if(st.addressable){uint32_t start=millis();while(!rmtTransmitCompleted(st.pin)&&millis()-start<10)delay(1);lightOff(k,true);rmtDeinit(st.pin);}else{lightOff(k,true);ledcDetach(st.pin);}pinMode(st.pin,INPUT);}stormAttached[k]=false;lightWasArmed[k]=false;stormEngines[k].reset();}}

void setupStorm(){for(unsigned k=0;k<2;k++){const auto& st=stormConfig(k);stormFaults[k]=false;lightWasArmed[k]=false;lightEvent[k]=false;stormEngines[k].reset();if(!st.enabled)continue;stormAttached[k]=st.addressable?rmtInit(st.pin,RMT_TX_MODE,RMT_MEM_NUM_BLOCKS_1,10000000):ledcAttach(st.pin,5000,8);stormFaults[k]=!stormAttached[k];lightOff(k,true);}}

uint32_t statusLEDColor(){return physicalStop.blocked()||servoFault||stormFaults[0]||stormFaults[1]?0xff0000:engine.running?0xffa000:engine.armed?0x00ff30:0x0060ff;}
void statusLEDTick(){if(!statusLED.enabled||!statusLEDAttached||!rmtTransmitCompleted(statusLED.pin))return;uint32_t color=statusLEDColor();if(color==statusLEDLast)return;unsigned n=0;for(unsigned pixel=0;pixel<statusLED.pixels;pixel++)for(unsigned byte=0;byte<3;byte++){uint8_t value=stormByte(color,byte,statusLED.brightness);for(int bit=7;bit>=0;bit--){bool one=value&(1<<bit);auto& item=statusLEDData[n++];item.level0=1;item.duration0=one?8:4;item.level1=0;item.duration1=one?4:8;}}statusLEDData[n].level0=statusLEDData[n].level1=0;statusLEDData[n].duration0=statusLEDData[n].duration1=3000;if(rmtWriteAsync(statusLED.pin,statusLEDData,n+1))statusLEDLast=color;else statusLEDFault=true;}
void lightingTick(uint32_t now){for(unsigned k=0;k<2;k++){const auto& st=stormConfig(k);if(!st.enabled||!stormAttached[k])continue;

 bool sequence=engine.running||engine.idle;bool muted=sequence&&!(config.scenes[engine.sceneIndex()].enabledMask&(1u<<(7+k)));
 bool manual=!engine.armed&&!sequence&&lightTesting[k]&&uint32_t(now-lightTestStart[k])<3000;if(!manual)lightTesting[k]=false;
 if((!engine.armed&&!sequence&&!manual)||muted){if(!lightingDark[k]){if(!st.addressable||rmtTransmitCompleted(st.pin)){lightOff(k,false);lightingDark[k]=true;}}lightWasArmed[k]=false;stormEngines[k].reset();continue;}

 if(!lightWasArmed[k]){lightWasArmed[k]=true;lightStarted[k]=now;}

 bool event=false;uint32_t t=now-lightStarted[k];if(engine.running||engine.idle){auto& sc=config.scenes[engine.sceneIndex()];auto elapsed=engine.elapsed(now);for(unsigned i=0;i<sc.lightCount;i++){auto& q=sc.lightCues[i];if(q.channel==k&&elapsed>=q.start&&elapsed-q.start<q.duration){event=true;t=elapsed-q.start;break;}}}

 if(manual){event=lightTestEvent[k];t=now-lightTestStart[k];}
 if(lightEvent[k]!=event){stormEngines[k].reset();lightEvent[k]=event;}

 const auto& p=event?st.event:st.idle;bool allowed=manual||event||st.ambient;bool flash=stormEngines[k].tick(st,allowed&&p.effect==2,now,esp_random(),k?esp_random()%501:0);

 if(uint32_t(now-lightLastFrame[k])<(p.effect==2?2:10))continue;if(st.addressable&&!rmtTransmitCompleted(st.pin))continue;lightLastFrame[k]=now;

 LightProfile physical=p;if(!st.addressable)physical.color=physical.secondary=0xffffff;

 for(unsigned pixel=0;pixel<st.pixels;pixel++)lightColors[k][pixel]=allowed?lightPixel(physical,t,pixel,st.pixels,flash,1931+k*7349):0;lightingDark[k]=false;

 if(st.addressable){stormFrame(k);if(!rmtWriteAsync(st.pin,stormData[k],st.pixels*24+1))stormFaults[k]=true;}

 else{uint32_t c=lightColors[k][0];unsigned b=max<unsigned>((c>>16)&255,max<unsigned>((c>>8)&255,c&255));if(!ledcWrite(st.pin,b))stormFaults[k]=true;}

}}

void writeOutputs(uint32_t now){bool requested[MAX_OUTPUTS]={};for(size_t i=0;i<config.outputCount;i++)requested[i]=engine.on(i,now);outputGuard.tick(config,requested,now);for(size_t i=0;i<config.outputCount;i++)if(!outputGuard.state[i])digitalWrite(config.outputs[i].pin,config.outputs[i].activeHigh?LOW:HIGH);for(size_t i=0;i<config.outputCount;i++)if(outputGuard.state[i])digitalWrite(config.outputs[i].pin,config.outputs[i].activeHigh?HIGH:LOW);lightingTick(now);}

void encodeProfile(JsonObject o,const LightProfile& p){o["effect"]=p.effect;o["brightness"]=p.brightness;o["speed"]=p.speed;o["width"]=p.width;o["color"]=p.color;o["secondary"]=p.secondary;}

void releasePins() {
  releaseStatusLED();

  releaseStorm();

  disableServo(); if (config.servo) pinMode(config.servoPin,INPUT);

  for (size_t i = 0; i < config.outputCount; ++i) {

    digitalWrite(config.outputs[i].pin, config.outputs[i].activeHigh ? LOW : HIGH);

    pinMode(config.outputs[i].pin, INPUT);

  }

  pinMode(config.triggerPin, INPUT);

  if (config.audio) { player.end(); pinMode(config.audioTx, INPUT); pinMode(config.audioRx, INPUT); }
 if(config.audio2){player2.end();pinMode(config.audio2Tx,INPUT);pinMode(config.audio2Rx,INPUT);}

}

void setupPins() {

  servoFault=false; if (config.servo) { digitalWrite(config.servoPin,LOW); pinMode(config.servoPin,OUTPUT); }

  for (size_t i = 0; i < config.outputCount; ++i) {

    digitalWrite(config.outputs[i].pin, config.outputs[i].activeHigh ? LOW : HIGH);

    pinMode(config.outputs[i].pin, OUTPUT);

  }

  pinMode(config.triggerPin, config.pull == 1 ? INPUT_PULLUP : config.pull == 2 ? INPUT_PULLDOWN : INPUT);

  if (config.audio) player.begin(9600, SERIAL_8N1, config.audioRx, config.audioTx);

  if(config.audio2)player2.begin(9600,SERIAL_8N1,config.audio2Rx,config.audio2Tx);
 for(auto& a:audioRuntime)a=AudioRuntime{};audioBootAt=millis();

  setupStorm();setupStatusLED();

  audioClock.reset();audioProgram=0;lastIdle=false;outputGuard.reset(); engine.configure(&config, millis(), digitalRead(config.triggerPin)); lastRun = false;

}

void defaults() {

  resetConfig(config);

  const auto& pins=BOARD_PINS;

  const char* names[] = {"Eyes / lighting","Prop motion","Fog / accent","Output 4","Output 5","Output 6"};

  for (int i = 0; i < BOARD_OUTPUTS; ++i) { config.outputs[i].pin = pins[i]; strlcpy(config.outputs[i].name, names[i], sizeof(config.outputs[i].name)); }

  Scene& s = config.scenes[0]; strlcpy(s.name, "Graveyard awakening", sizeof(s.name)); s.count = 5;

  s.cues[0] = {0,0,10000}; s.cues[1] = {1,1500,1200}; s.cues[2] = {2,3000,2200};

  s.cues[3] = {1,6000,700}; s.cues[4] = {1,7500,1000};

}

void encodeConfig(JsonDocument& doc, const Config& c) {

 JsonObject st=doc["storm"].to<JsonObject>();st["pin"]=c.storm.pin;encodeProfile(st["idle"].to<JsonObject>(),c.storm.idle);encodeProfile(st["event"].to<JsonObject>(),c.storm.event);st["enabled"]=c.storm.enabled;st["addressable"]=c.storm.addressable;st["ambient"]=c.storm.ambient;st["channel"]=c.storm.channel;st["pixels"]=c.storm.pixels;st["brightness"]=c.storm.brightness;st["bursts"]=c.storm.bursts;st["color"]=c.storm.color;st["minWait"]=c.storm.minWait;st["maxWait"]=c.storm.maxWait;st["flashMs"]=c.storm.flashMs;st["gapMs"]=c.storm.gapMs;

 {

 JsonObject st=doc["storm2"].to<JsonObject>();st["pin"]=c.storm2.pin;encodeProfile(st["idle"].to<JsonObject>(),c.storm2.idle);encodeProfile(st["event"].to<JsonObject>(),c.storm2.event);st["enabled"]=c.storm2.enabled;st["addressable"]=c.storm2.addressable;st["ambient"]=c.storm2.ambient;st["channel"]=c.storm2.channel;st["pixels"]=c.storm2.pixels;st["brightness"]=c.storm2.brightness;st["bursts"]=c.storm2.bursts;st["color"]=c.storm2.color;st["minWait"]=c.storm2.minWait;st["maxWait"]=c.storm2.maxWait;st["flashMs"]=c.storm2.flashMs;st["gapMs"]=c.storm2.gapMs;

 }

  JsonObject sv=doc["servo"].to<JsonObject>();

  sv["enabled"]=c.servo; sv["pin"]=c.servoPin; sv["minUs"]=c.servoMinUs; sv["maxUs"]=c.servoMaxUs; sv["minAngle"]=c.servoMinAngle; sv["maxAngle"]=c.servoMaxAngle; sv["home"]=c.servoHome;

  JsonObject repeat = doc["repeat"].to<JsonObject>();

  repeat["enabled"]=c.repeat; repeat["min"]=c.repeatMin; repeat["max"]=c.repeatMax;

  JsonObject motion = doc["motion"].to<JsonObject>();

  motion["kind"]=c.pneumaticInterlock?"pneumatic":"motor"; motion["a"]=c.interlockA; motion["b"]=c.interlockB; motion["deadTime"]=c.deadTime;

  auto behavior=doc["behavior"].to<JsonObject>();behavior["random"]=c.randomScenes;behavior["idle"]=c.idleScene;behavior["idleGap"]=c.idleGap;
  doc["schema"] = 1; doc["selected"] = c.selected;

  JsonObject tr = doc["trigger"].to<JsonObject>();

  tr["enabled"] = c.triggerEnabled; tr["pin"] = c.triggerPin; tr["activeHigh"] = c.triggerHigh; tr["pull"] = c.pull;

  tr["debounce"] = c.debounce; tr["cooldown"] = c.cooldown;

  JsonObject au = doc["audio"].to<JsonObject>();

  au["player"]=c.audioDY?"dy":"dfplayer"; au["enabled"] = c.audio; au["tx"] = c.audioTx; au["rx"] = c.audioRx;

  au["volume"] = c.volume; au["ambientTrack"] = c.ambientTrack;

  JsonObject au2=doc["audio2"].to<JsonObject>();au2["enabled"]=c.audio2;au2["player"]=c.audio2DY?"dy":"dfplayer";au2["tx"]=c.audio2Tx;au2["rx"]=c.audio2Rx;au2["volume"]=c.volume2;
  JsonArray outputs = doc["outputs"].to<JsonArray>();

  for (size_t i = 0; i < c.outputCount; ++i) {

    JsonObject out = outputs.add<JsonObject>();

    out["maxOnMs"] = c.outputs[i].maxOnMs; out["name"] = c.outputs[i].name; out["pin"] = c.outputs[i].pin; out["activeHigh"] = c.outputs[i].activeHigh;

  }

  JsonArray scenes = doc["scenes"].to<JsonArray>();

  for (size_t i = 0; i < c.sceneCount; ++i) {

    const Scene& s = c.scenes[i]; JsonObject obj = scenes.add<JsonObject>();

    obj["enabledMask"]=s.enabledMask;obj["name"] = s.name; obj["duration"] = s.duration; obj["scareTrack"] = s.scareTrack;

    JsonArray lights=obj["lightCues"].to<JsonArray>();for(unsigned j=0;j<s.lightCount;j++){auto o=lights.add<JsonObject>();o["channel"]=s.lightCues[j].channel;o["start"]=s.lightCues[j].start;o["duration"]=s.lightCues[j].duration;}

    auto audio=obj["audioCues"].to<JsonArray>();for(unsigned j=0;j<s.audioCount;j++){auto o=audio.add<JsonObject>();const auto& q=s.audioCues[j];o["start"]=q.start;o["player"]=q.player;o["action"]=q.action;o["value"]=q.value;}
    obj["randomServo"]=s.randomServo;
    JsonArray moves=obj["servoMoves"].to<JsonArray>();

    for (size_t j=0;j<s.servoCount;++j) { JsonObject move=moves.add<JsonObject>(); move["start"]=s.servoMoves[j].start; move["duration"]=s.servoMoves[j].duration; move["angle"]=s.servoMoves[j].angle;move["ease"]=s.servoMoves[j].ease; }

    JsonArray cues = obj["cues"].to<JsonArray>();

    for (size_t j = 0; j < s.count; ++j) {

      JsonObject cue = cues.add<JsonObject>(); cue["output"] = s.cues[j].output;

      cue["start"] = s.cues[j].start; cue["duration"] = s.cues[j].duration;

    }

  }

}

bool number(JsonVariantConst v, int lo, int hi) { return v.is<int>() && v.as<int>() >= lo && v.as<int>() <= hi; }

bool nameField(JsonVariantConst v, size_t max) { return v.is<const char*>() && strlen(v.as<const char*>()) > 0 && strlen(v.as<const char*>()) <= max; }

bool decodeProfile(JsonVariantConst o,LightProfile& p){if(!number(o["effect"],0,10)||!number(o["brightness"],0,100)||!number(o["speed"],100,10000)||!number(o["width"],1,20)||!number(o["color"],0,0xffffff)||!number(o["secondary"],0,0xffffff))return false;p.effect=o["effect"];p.brightness=o["brightness"];p.speed=o["speed"];p.width=o["width"];p.color=o["color"];p.secondary=o["secondary"];return true;}

const char* decodeConfig(JsonVariantConst root, Config& c) {
  if(const auto* error=validateConfigFields(root))return error;

  if (!root.is<JsonObjectConst>() || !number(root["schema"],1,1)) return "Unsupported cue-sheet format.";

  JsonArrayConst outs = root["outputs"].as<JsonArrayConst>();

  JsonArrayConst scenes = root["scenes"].as<JsonArrayConst>();

  if (!outs || outs.size() < 1 || outs.size() > BOARD_OUTPUTS || !scenes || scenes.size() < 1 || scenes.size() > MAX_SCENES) return "Use 1–6 outputs and 1–4 scenes.";

  auto tr = root["trigger"]; auto au = root["audio"];

  if (!number(root["selected"],0,scenes.size()-1) || !number(tr["pin"],0,39) || !tr["activeHigh"].is<bool>() || !number(tr["pull"],0,2) || !number(tr["debounce"],5,1000) || !number(tr["cooldown"],0,60000)) return "Invalid trigger settings.";

  if (!au["enabled"].is<bool>() || !number(au["tx"],0,39) || !number(au["rx"],0,39) || !number(au["volume"],0,30) || !number(au["ambientTrack"],0,2999)) return "Invalid audio settings.";

  if(!au["player"].isNull() && au["player"]!="dfplayer" && au["player"]!="dy")return "Unknown audio player.";

  resetConfig(c); c.outputCount = outs.size(); c.sceneCount = scenes.size(); c.selected = root["selected"]; c.storm.channel=min<uint8_t>(5,c.outputCount-1); c.storm2.channel=min<uint8_t>(4,c.outputCount-1);

  if (!root["storm"].isNull()) {

 auto st=root["storm"];if(!st["enabled"].is<bool>()||!st["addressable"].is<bool>()||!st["ambient"].is<bool>()||!number(st["channel"],0,5)||!number(st["pixels"],1,60)||!number(st["brightness"],1,100)||!number(st["bursts"],1,8)||!number(st["color"],0,0xFFFFFF)||!number(st["minWait"],500,60000)||!number(st["maxWait"],500,60000)||!number(st["flashMs"],10,300)||!number(st["gapMs"],20,500))return "Invalid lightning settings.";

 c.storm.enabled=st["enabled"];c.storm.addressable=st["addressable"];c.storm.ambient=st["ambient"];c.storm.channel=st["channel"];c.storm.pixels=st["pixels"];c.storm.brightness=st["brightness"];c.storm.bursts=st["bursts"];c.storm.color=st["color"];c.storm.minWait=st["minWait"];c.storm.maxWait=st["maxWait"];c.storm.flashMs=st["flashMs"];c.storm.gapMs=st["gapMs"];if(st["pin"].isNull()){c.storm.enabled=false;c.storm.idle.color=c.storm.color;c.storm.idle.brightness=c.storm.brightness;c.storm.event.effect=5;}else{if(!number(st["pin"],0,39)||!decodeProfile(st["idle"],c.storm.idle)||!decodeProfile(st["event"],c.storm.event))return "Invalid lighting profile.";c.storm.pin=st["pin"];}

 }

  if (!root["storm2"].isNull()) {

 auto st=root["storm2"];if(!st["enabled"].is<bool>()||!st["addressable"].is<bool>()||!st["ambient"].is<bool>()||!number(st["channel"],0,5)||!number(st["pixels"],1,60)||!number(st["brightness"],1,100)||!number(st["bursts"],1,8)||!number(st["color"],0,0xFFFFFF)||!number(st["minWait"],500,60000)||!number(st["maxWait"],500,60000)||!number(st["flashMs"],10,300)||!number(st["gapMs"],20,500))return "Invalid lightning settings.";

 c.storm2.enabled=st["enabled"];c.storm2.addressable=st["addressable"];c.storm2.ambient=st["ambient"];c.storm2.channel=st["channel"];c.storm2.pixels=st["pixels"];c.storm2.brightness=st["brightness"];c.storm2.bursts=st["bursts"];c.storm2.color=st["color"];c.storm2.minWait=st["minWait"];c.storm2.maxWait=st["maxWait"];c.storm2.flashMs=st["flashMs"];c.storm2.gapMs=st["gapMs"];if(st["pin"].isNull()){c.storm2.enabled=false;c.storm2.idle.color=c.storm2.color;c.storm2.idle.brightness=c.storm2.brightness;c.storm2.event.effect=5;}else{if(!number(st["pin"],0,39)||!decodeProfile(st["idle"],c.storm2.idle)||!decodeProfile(st["event"],c.storm2.event))return "Invalid lighting profile.";c.storm2.pin=st["pin"];}

 }

  if (!root["servo"].isNull()) {

    auto v=root["servo"]; if (!v["enabled"].is<bool>() || !number(v["pin"],0,39) || !number(v["minUs"],500,2500) || !number(v["maxUs"],500,2500) || !number(v["minAngle"],0,180) || !number(v["maxAngle"],0,180) || !number(v["home"],0,180)) return "Invalid servo settings.";

    c.servo=v["enabled"];c.servoPin=v["pin"];c.servoMinUs=v["minUs"];c.servoMaxUs=v["maxUs"];c.servoMinAngle=v["minAngle"];c.servoMaxAngle=v["maxAngle"];c.servoHome=v["home"];

  }

  if (!root["repeat"].isNull()) {

    auto r=root["repeat"]; if (!r["enabled"].is<bool>() || !number(r["min"],1000,3600000) || !number(r["max"],1000,3600000)) return "Invalid repeat settings.";

    c.repeat=r["enabled"]; c.repeatMin=r["min"]; c.repeatMax=r["max"];

  }

  if (!root["motion"].isNull()) {

    auto m=root["motion"]; if (!number(m["a"],-1,5) || !number(m["b"],-1,5) || !number(m["deadTime"],20,2000)) return "Invalid motion interlock.";

    const char* kind=m["kind"] | "motor"; if(strcmp(kind,"motor")&&strcmp(kind,"pneumatic"))return "Invalid interlock type."; c.pneumaticInterlock=!strcmp(kind,"pneumatic"); c.interlockA=m["a"]; c.interlockB=m["b"]; c.deadTime=m["deadTime"];

  }

  if(!root["behavior"].isNull()){auto b=root["behavior"];if(!b["random"].is<bool>()||!number(b["idle"],-1,c.sceneCount-1)||!number(b["idleGap"],1000,3600000))return "Invalid scene behavior.";c.randomScenes=b["random"];c.idleScene=b["idle"];c.idleGap=b["idleGap"];}
  if(!tr["enabled"].isNull()&&!tr["enabled"].is<bool>())return "Invalid trigger enable setting."; c.triggerEnabled=tr["enabled"].isNull()?true:tr["enabled"].as<bool>(); c.triggerPin = tr["pin"]; c.triggerHigh = tr["activeHigh"]; c.pull = tr["pull"];

  c.debounce = tr["debounce"]; c.cooldown = tr["cooldown"];

  if(!root["audio2"].isNull()){auto v=root["audio2"];if(!v["enabled"].is<bool>()||!number(v["tx"],0,39)||!number(v["rx"],0,39)||!number(v["volume"],0,30)||(v["player"]!="dfplayer"&&v["player"]!="dy"))return "Invalid player 2 settings.";c.audio2=v["enabled"];c.audio2DY=v["player"]=="dy";c.audio2Tx=v["tx"];c.audio2Rx=v["rx"];c.volume2=v["volume"];}
  c.audioDY=au["player"]=="dy"; c.audio = au["enabled"]; c.audioTx = au["tx"]; c.audioRx = au["rx"]; c.volume = au["volume"]; c.ambientTrack = au["ambientTrack"];

  for (size_t i = 0; i < outs.size(); ++i) {

    auto o = outs[i]; if (!nameField(o["name"],24) || !number(o["pin"],0,39) || !o["activeHigh"].is<bool>()) return "Invalid output settings.";

    if (!o["maxOnMs"].isNull() && !number(o["maxOnMs"],0,120000)) return "Invalid pulse limit.";

    c.outputs[i].maxOnMs=o["maxOnMs"] | 0;

    strlcpy(c.outputs[i].name,o["name"],sizeof(c.outputs[i].name)); c.outputs[i].pin = o["pin"]; c.outputs[i].activeHigh = o["activeHigh"];

  }

  for (size_t i = 0; i < scenes.size(); ++i) {

    auto s = scenes[i]; auto cues = s["cues"].as<JsonArrayConst>();

    if (!nameField(s["name"],32) || !number(s["duration"],1,MAX_DURATION) || !number(s["scareTrack"],0,2999) || !cues || cues.size() > MAX_CUES) return "Invalid scene settings.";

    if(!s["randomServo"].isNull()&&!s["randomServo"].is<bool>())return "Invalid random servo setting.";
    Scene& scene = c.scenes[i];scene.enabledMask=s["enabledMask"] | 2047;scene.randomServo=s["randomServo"] | false; strlcpy(scene.name,s["name"],sizeof(scene.name)); scene.duration = s["duration"]; scene.scareTrack = s["scareTrack"]; scene.count = cues.size();

    if(!s["audioCues"].isNull()){auto a=s["audioCues"].as<JsonArrayConst>();if(!a||a.size()>8)return "Invalid audio cues.";scene.audioCount=a.size();for(unsigned j=0;j<a.size();j++){auto q=a[j];if(!number(q["start"],0,MAX_DURATION)||!number(q["player"],0,1)||!number(q["action"],0,2)||!number(q["value"],0,2999))return "Invalid audio cue.";scene.audioCues[j]={q["start"].as<uint32_t>(),q["player"].as<uint8_t>(),q["action"].as<uint8_t>(),q["value"].as<uint16_t>()};}}
    if(!s["lightCues"].isNull()){auto lights=s["lightCues"].as<JsonArrayConst>();if(!lights||lights.size()>16)return "Invalid lighting cues.";scene.lightCount=lights.size();for(unsigned j=0;j<lights.size();j++){auto q=lights[j];if(!number(q["channel"],0,1)||!number(q["start"],0,MAX_DURATION)||!number(q["duration"],1,MAX_DURATION))return "Invalid lighting cue.";scene.lightCues[j]={q["channel"].as<uint8_t>(),q["start"].as<uint32_t>(),q["duration"].as<uint32_t>()};}}

    if (!s["servoMoves"].isNull()) {

      auto moves=s["servoMoves"].as<JsonArrayConst>(); if (!moves || moves.size()>MAX_SERVO_MOVES) return "Invalid servo movements.";

      scene.servoCount=moves.size();

      for (size_t j=0;j<moves.size();++j) { auto m=moves[j]; if (!number(m["start"],0,MAX_DURATION) || !number(m["duration"],1,MAX_DURATION) || !number(m["angle"],0,180)) return "Invalid servo movement."; if(!m["ease"].isNull()&&!m["ease"].is<bool>())return "Invalid servo easing.";scene.servoMoves[j]={m["start"].as<uint32_t>(),m["duration"].as<uint32_t>(),m["angle"].as<uint8_t>(),m["ease"] | false}; }

    }

    for (size_t j = 0; j < cues.size(); ++j) {

      auto cue = cues[j];

      if (!number(cue["output"],0,c.outputCount-1) || !number(cue["start"],0,MAX_DURATION) || !number(cue["duration"],1,MAX_DURATION)) return "Invalid cue timing.";

      scene.cues[j] = {cue["output"].as<uint8_t>(),cue["start"].as<uint32_t>(),cue["duration"].as<uint32_t>()};

    }

  }

  return validate(c);

}

bool readConfig(const char* path, Config& out) {

  File file = LittleFS.open(path,"r"); if (!file || file.size() > MAX_CONFIG_BYTES) return false;

  JsonDocument doc; auto err = deserializeJson(doc,file); file.close();

  return !err && !decodeConfig(doc.as<JsonVariantConst>(),out);

}

bool persistConfig(const Config& next) {

  if (!fsOK) return false;

  JsonDocument doc; encodeConfig(doc,next);

  File file = LittleFS.open("/config.tmp","w"); if (!file) return false;

  size_t expected = measureJson(doc); size_t written = serializeJson(doc,file); file.flush(); file.close();

  // HTTP saves are serialized on the loop task; avoid a second large Config on its stack.

  #if defined(CONFIG_IDF_TARGET_ESP32S2)
  std::unique_ptr<Config> verifyMemory(new(std::nothrow) Config);if(!verifyMemory)return false;Config& verify=*verifyMemory;
#else
  static Config verify;
#endif
 if (written != expected || !readConfig("/config.tmp",verify)) return false;

  LittleFS.remove("/config.bak");

  if (LittleFS.exists("/config.json") && !LittleFS.rename("/config.json","/config.bak")) return false;

  return LittleFS.rename("/config.tmp","/config.json");

}

void jsonReply(const JsonDocument& doc) { String payload; serializeJson(doc,payload); server.send(200,"application/json",payload); }

void errorReply(int code, const char* message) { JsonDocument doc; doc["error"] = message;if(message==validationMessage&&validationPath[0])doc["field"]=validationPath; String p; serializeJson(doc,p); server.send(code,"application/json",p); }

bool authorized(bool multipart=false) {
  if(server.method()==HTTP_POST&&otaBusy&&server.uri()!="/api/stop"&&server.uri()!="/api/update"){errorReply(409,"Firmware update in progress. Outputs remain disarmed.");return false;}

  if (!multipart && server.method()==HTTP_POST && !server.header("Content-Type").startsWith("application/json")) {

    errorReply(415,"Use application/json for controller commands."); return false;

  }

  if (server.authenticate("admin",adminPassword.c_str())) return true;

  server.requestAuthentication(DIGEST_AUTH,"HauntCue","Log in with admin and your controller password."); return false;

}

bool stoppedForEdit() {

  if (otaBusy || engine.armed || engine.running) { errorReply(409,"Disarm before saving or testing outputs."); return false; }

  return true;

}

void halt() {

  for(auto& st:stormEngines)st.reset();

  disableServo();

  lightTesting[0]=lightTesting[1]=false;engine.arm(false,millis(),digitalRead(config.triggerPin)); writeOutputs(millis()); lastRun = false; stopAudio();

}

bool physicalMotionAllowed(){if(otaBusy||configLoadFault||!fsOK){errorReply(409,"Motion blocked: update or settings recovery is in progress. Saved settings have not been replaced.");return false;}samplePhysicalStop(millis());if(physicalStop.blocked()){halt();errorReply(409,"Physical stop latched. Release the button, reset the stop, then arm explicitly.");return false;}return true;}
void physicalStopReply(){JsonDocument d;d["enabled"]=physicalStop.enabled;d["pin"]=physicalStopPin;d["latched"]=physicalStop.latched;d["open"]=physicalStop.open;d["ready"]=physicalStop.ready(millis());d["conflict"]=physicalStop.enabled&&physicalStopConflict(config,physicalStopPin);jsonReply(d);}
void statusLEDReply(){JsonDocument d;d["enabled"]=statusLED.enabled;d["pin"]=statusLED.pin;d["pixels"]=statusLED.pixels;d["brightness"]=statusLED.brightness;d["fault"]=statusLEDFault;d["color"]=statusLEDColor();jsonReply(d);}
bool armPinSupported(int pin){return (inputPin(pin)||(strcmp(BOARD_FAMILY,"esp32")==0&&pin==17))&&!boardNeedsExternalPull(pin);}
bool armPinConflict(const Config& c,int pin){return physicalStopConflict(c,pin)||(physicalStop.enabled&&physicalStopPin==pin)||(statusLED.enabled&&statusLED.pin==pin);}
void configureArmPin(){if(armInput.enabled&&!armPinConflict(config,armInputPin)){pinMode(armInputPin,INPUT_PULLUP);armInput.configure(true,armInput.toggle,millis(),digitalRead(armInputPin)==LOW);}}
void sampleArmPin(uint32_t now){
 if(!armInput.enabled)return;
 if(armPinConflict(config,armInputPin)){halt();return;}
 const int event=armInput.sample(digitalRead(armInputPin)==LOW,now);
 if(event<0||(event>0&&armInput.toggle&&engine.armed)){halt();return;}
 if(event>0&&!engine.armed&&!engine.running&&!otaBusy&&!configLoadFault&&fsOK&&!physicalStop.blocked()){
  disableServo();engine.arm(true,now,digitalRead(config.triggerPin));writeOutputs(now);lastRun=false;ambient();
 }
}
void armInputReply(){JsonDocument d;d["enabled"]=armInput.enabled;d["pin"]=armInputPin;d["toggle"]=armInput.toggle;d["pressed"]=armInput.active();auto pins=d["pins"].to<JsonArray>();for(int p=0;p<=48;p++)if(armPinSupported(p))pins.add(p);jsonReply(d);}
#include "FirmwareUpdate.h"
void initRoutes() {
 installUpdateRoutes();
 server.on("/api/arm-input",HTTP_GET,[]{if(authorized()){EngineLock lock;armInputReply();}});
 server.on("/api/arm-input",HTTP_POST,[]{
  if(!authorized())return;JsonDocument d;
  if(server.arg("plain").length()>200||deserializeJson(d,server.arg("plain"))||!d["enabled"].is<bool>()||!d["toggle"].is<bool>()||!number(d["pin"],-1,48)||(d["enabled"].as<bool>()&&!armPinSupported(d["pin"]))){errorReply(400,"Arm input: select a supported GPIO with an internal pull-up and a button mode.");return;}
  EngineLock lock;if(!stoppedForEdit())return;
  if(d["enabled"].as<bool>()&&armPinConflict(config,d["pin"])){errorReply(409,"Arm input GPIO is already used by a prop, trigger, physical stop or status light. Free and save it first.");return;}
  String payload;serializeJson(d,payload);
  if(!nvsOK||prefs.putString("arm-input",payload)!=payload.length()){errorReply(507,"Arm input was not saved. Previous settings remain active.");return;}
  halt();if(armInput.enabled)pinMode(armInputPin,INPUT);
  armInputPin=d["pin"];armInput.configure(d["enabled"],d["toggle"],millis(),false);configureArmPin();armInputReply();
 });
 server.on("/api/sequence-test",HTTP_POST,[]{
  if(!authorized())return;JsonDocument d;if(server.arg("plain").length()>100||deserializeJson(d,server.arg("plain"))||!number(d["scene"],0,config.sceneCount-1)){errorReply(400,"Choose an existing scene for a one-shot sequence test.");return;}
  EngineLock lock;if(!stoppedForEdit()||!physicalMotionAllowed())return;
  halt();if(!engine.preview(d["scene"],millis())){errorReply(409,"Wait for the current test to finish.");return;}
  writeOutputs(millis());scare();lastRun=true;server.send(200,"application/json","{\"ok\":true}");
 });
 server.on("/api/status-led",HTTP_GET,[]{if(!authorized())return;EngineLock lock;statusLEDReply();});
 server.on("/api/status-led",HTTP_POST,[]{if(!authorized())return;JsonDocument d;if(server.arg("plain").length()>200||deserializeJson(d,server.arg("plain"))||!d["enabled"].is<bool>()||!number(d["pin"],0,48)||!statusPinSupported(d["pin"])||!number(d["pixels"],1,8)||!number(d["brightness"],1,100)){errorReply(400,"Check status LED GPIO, 1–8 pixels and brightness 1–100%.");return;}EngineLock lock;if(!stoppedForEdit())return;if(d["enabled"].as<bool>()&&statusLEDConflict(config,d["pin"])){errorReply(409,"Status LEDs need a free GPIO, separate from prop devices and the stop button.");return;}auto previous=statusLED;releaseStatusLED();statusLED.enabled=d["enabled"];statusLED.pin=d["pin"];statusLED.pixels=d["pixels"];statusLED.brightness=d["brightness"];setupStatusLED();if(statusLEDFault){releaseStatusLED();statusLED=previous;setupStatusLED();errorReply(409,"No LED driver resource available. Prop lighting retains priority; disable an addressable prop channel or this status strip.");return;}String payload;serializeJson(d,payload);if(!nvsOK||prefs.putString("status-led",payload)!=payload.length()){releaseStatusLED();statusLED=previous;setupStatusLED();errorReply(507,"Status LED settings were not saved.");return;}statusLEDTick();statusLEDReply();});

 server.on("/api/physical-stop",HTTP_GET,[]{if(!authorized())return;EngineLock lock;samplePhysicalStop(millis());physicalStopReply();});
 server.on("/api/physical-stop",HTTP_POST,[]{
  if(!authorized())return;JsonDocument d;
  if(server.arg("plain").length()>200||deserializeJson(d,server.arg("plain"))||!d["enabled"].is<bool>()||!number(d["pin"],0,48)||!inputPin(d["pin"])){errorReply(400,"Choose a supported emergency-stop input pin.");return;}
  EngineLock lock;if(!stoppedForEdit())return;const bool enabled=d["enabled"];const int pin=d["pin"];
  if(enabled&&(physicalStopConflict(config,pin)||(statusLED.enabled&&statusLED.pin==pin)||(armInput.enabled&&armInputPin==pin))){errorReply(409,"That pin belongs to a prop device. Free it before enabling the physical stop.");return;}
  if(physicalStop.enabled&&!enabled&&d["confirmDisable"]!=true){errorReply(400,"Confirm disabling physical stop protection with prop power disconnected.");return;}
  if(!nvsOK){errorReply(507,"Controller safety settings storage unavailable.");return;}
  JsonDocument stored;stored["enabled"]=enabled;stored["pin"]=pin;String payload;serializeJson(stored,payload);
  halt();if(prefs.putString("physical-stop",payload)!=payload.length()){errorReply(507,"Safety settings were not saved.");return;}
  if(physicalStop.enabled&&!physicalStopConflict(config,physicalStopPin))pinMode(physicalStopPin,INPUT);
  physicalStopPin=pin;physicalStop.configure(enabled,millis());configurePhysicalStopPin();samplePhysicalStop(millis());physicalStopReply();
 });
 server.on("/api/physical-stop-reset",HTTP_POST,[]{if(!authorized())return;EngineLock lock;samplePhysicalStop(millis());if(!physicalStop.reset(millis())){errorReply(409,"Close the normally closed button contact for at least 100 ms before resetting.");return;}halt();physicalStopReply();});


  server.on("/api/setup-complete",HTTP_POST,[] {

    if(!authorized())return;{EngineLock lock;if(!stoppedForEdit())return;}

    if(!nvsOK||!prefs.putBool("setup",true)){errorReply(507,"Could not save setup status.");return;}

    server.send(200,"application/json","{\"ok\":true}");

  });





  server.on("/api/capabilities",HTTP_GET,[]{if(!authorized())return;JsonDocument d;d["family"]=BOARD_FAMILY;d["maxCues"]=MAX_CUES;d["maxConfigBytes"]=MAX_CONFIG_BYTES;d["maxOutputs"]=BOARD_OUTPUTS;d["audioPlayers"]=BOARD_AUDIO;d["trigger"]=BOARD_TRIGGER;d["servo"]=BOARD_SERVO;d["light1"]=BOARD_LIGHT1;d["light2"]=BOARD_LIGHT2;d["tx"]=BOARD_TX;d["rx"]=BOARD_RX;d["tx2"]=BOARD_TX2;d["rx2"]=BOARD_RX2;auto o=d["outputs"].to<JsonArray>();for(int p:BOARD_PINS)o.add(p);auto op=d["outputPins"].to<JsonArray>();auto ip=d["inputPins"].to<JsonArray>();for(int p=0;p<=48;p++){if(outputPin(p))op.add(p);if(inputPin(p))ip.add(p);}jsonReply(d);});
  server.on("/api/library",HTTP_GET,[]{if(!authorized())return;if(!fsOK||!LittleFS.exists("/library.json")){server.send(200,"application/json","{}");return;}File f=LittleFS.open("/library.json","r");if(!f){errorReply(500,"Inventory read failed.");return;}server.streamFile(f,"application/json");f.close();});
  server.on("/api/library",HTTP_POST,[]{if(!authorized())return;{EngineLock lock;if(!stoppedForEdit())return;}if(!fsOK){errorReply(507,"Filesystem unavailable.");return;}const String payload=server.arg("plain");JsonDocument d;if(payload.length()>32000||deserializeJson(d,payload)||d["schema"]!=1||!d["devices"].is<JsonArray>()||d["devices"].size()>32||!d["assignments"].is<JsonObject>()||!d["activeProp"].is<const char*>()||strlen(d["activeProp"].as<const char*>())>160){errorReply(400,"Invalid inventory or size limit.");return;}for(JsonObjectConst p:d["devices"].as<JsonArrayConst>()){if(p["schema"]!=1||!nameField(p["id"],64)||!nameField(p["name"],24)||!nameField(p["type"],32)||!p["settings"].is<JsonObjectConst>()){errorReply(400,"Invalid device profile.");return;}}File f=LittleFS.open("/library.tmp","w");if(!f){errorReply(507,"Inventory save failed.");return;}size_t n=f.print(payload);f.flush();f.close();if(n!=payload.length()){LittleFS.remove("/library.tmp");errorReply(507,"Incomplete inventory save.");return;}if(!LittleFS.rename("/library.tmp","/library.json")){errorReply(507,"Inventory replace failed.");return;}server.send(200,"application/json","{\"ok\":true}");});
  server.on("/api/stage",HTTP_GET,[] {

    if (!authorized()) return;

    if (!fsOK || !LittleFS.exists("/stage.json")) {server.send(200,"application/json","{}");return;}

    File f=LittleFS.open("/stage.json","r"); if(!f){errorReply(500,"Layout read failed.");return;}server.streamFile(f,"application/json");f.close();

  });

  server.on("/api/stage",HTTP_POST,[] {

    if (!authorized()) return;

    {EngineLock lock;if(!stoppedForEdit())return;}

    if(!fsOK){errorReply(507,"Filesystem unavailable.");return;}

    const String payload=server.arg("plain");JsonDocument d;

    if(payload.length()>16000||deserializeJson(d,payload)||d["schema"]!=1||!d["parts"].is<JsonArray>()||!d["lines"].is<JsonArray>()||d["parts"].size()>32||d["lines"].size()>80){errorReply(400,"Invalid layout or size limit.");return;}

    File f=LittleFS.open("/stage.tmp","w");if(!f){errorReply(507,"Layout save failed.");return;}

    size_t written=f.print(payload);f.flush();f.close();if(written!=payload.length()){LittleFS.remove("/stage.tmp");errorReply(507,"Incomplete layout save.");return;}

    if(!LittleFS.rename("/stage.tmp","/stage.json")){errorReply(507,"Layout replace failed.");return;}

    server.send(200,"application/json","{\"ok\":true}");

  });



  server.on("/api/name",HTTP_POST,[] {

    if (!authorized()) return;

    JsonDocument d; if (server.arg("plain").length()>200 || deserializeJson(d,server.arg("plain")) || !nameField(d["name"],32)) { errorReply(400,"Choose a controller name of 1–32 bytes."); return; }

    if (!nvsOK) { errorReply(507,"Network storage unavailable."); return; }

    JsonDocument network; network["name"]=d["name"];network["ssid"]=wifiSSID;network["wifi"]=wifiPassword;network["admin"]=adminPassword;

    String payload;serializeJson(network,payload);if(prefs.putString("network",payload)!=payload.length()){errorReply(507,"Name save failed.");return;}

    deviceName=d["name"].as<String>();if(discoveryReady)MDNS.addServiceTxt("hauntcue","tcp","name",deviceName);

    server.send(200,"application/json","{\"ok\":true}");

  });

  server.on("/api/peers",HTTP_GET,[] {

    if (!authorized()) return;

    if (WiFi.status()!=WL_CONNECTED) { errorReply(409,"Join your home Wi-Fi to discover other controllers."); return; }

    { EngineLock lock; if (engine.armed || servoAttached) { errorReply(409,"Disarm and release live servo control before scanning."); return; } }

    if (!discoveryReady) { errorReply(503,"Network discovery is unavailable."); return; }

    JsonDocument doc; JsonArray peers=doc["controllers"].to<JsonArray>();

    int count=MDNS.queryService("hauntcue","tcp");

    for (int i=0;i<count && peers.size()<32;++i) {

      String host=MDNS.hostname(i); if (host==deviceHost || MDNS.address(i)==WiFi.localIP()) continue;

      IPAddress ip=MDNS.address(i); if (ip==IPAddress(0,0,0,0)) continue;

      JsonObject peer=peers.add<JsonObject>();peer["name"]=MDNS.hasTxt(i,"name")?MDNS.txt(i,"name"):host;

      peer["host"]=host;peer["ip"]=ip.toString();peer["port"]=MDNS.port(i);peer["version"]=MDNS.hasTxt(i,"version")?MDNS.txt(i,"version"):"";

    }

    jsonReply(doc);

  });

  server.on("/api/servo",HTTP_POST,[] {

    if (!authorized()) return;

    JsonDocument doc; if (server.arg("plain").length()>200 || deserializeJson(doc,server.arg("plain"))) { errorReply(400,"Invalid servo command."); return; }

    if (!doc["active"].is<bool>() || (doc["active"].as<bool>() && !number(doc["angle"],0,180))) { errorReply(400,"Invalid servo angle."); return; }

    { EngineLock lock;

      if (!config.servo || engine.armed || engine.running) { errorReply(409,"Enable and save servo settings, then disarm for live control."); return; }

      if (doc["active"].as<bool>()) {
        if(!physicalMotionAllowed())return;

        int angle=doc["angle"];if(angle<config.servoMinAngle || angle>config.servoMaxAngle) {errorReply(400,"Angle exceeds saved travel limits.");return;}

        servoLive=true;servoLiveAt=millis();servoLiveAngle=angle;servoTick(millis());

      } else disableServo();

    }

    server.send(200,"application/json","{\"ok\":true}");

  });

  server.on("/",HTTP_GET,[] { if (authorized()) { server.sendHeader("Cache-Control","no-store"); server.sendHeader("Content-Encoding","gzip"); server.send_P(200,"text/html; charset=utf-8",WEB_UI,WEB_UI_LENGTH); } });

  server.on("/api/config",HTTP_GET,[] { if (!authorized()) return; JsonDocument doc; { EngineLock lock; encodeConfig(doc,config); } jsonReply(doc); });

  server.on("/api/config",HTTP_POST,[] {

    if (!authorized()) return;

    { EngineLock lock; if (!stoppedForEdit()) return; halt(); }

    if (server.arg("plain").length() > MAX_CONFIG_BYTES) { errorReply(413,"Cue sheet exceeds the 65536-byte settings limit. Export the draft and reduce it before saving."); return; }

    JsonDocument doc; if (deserializeJson(doc,server.arg("plain"))) { errorReply(400,"Invalid JSON."); return; }

    #if defined(CONFIG_IDF_TARGET_ESP32S2)
    std::unique_ptr<Config> nextMemory(new(std::nothrow) Config);if(!nextMemory){errorReply(507,"Not enough memory to edit settings.");return;}Config& next=*nextMemory;
#else
    static Config next;
#endif
 const char* error = decodeConfig(doc.as<JsonVariantConst>(),next);

    if (error) { errorReply(400,error); return; }
    if(armInput.enabled&&armPinConflict(next,armInputPin)){errorReply(409,"Arm input GPIO is reserved on this controller. Choose another prop GPIO.");return;}
    if(statusLED.enabled&&statusLEDConflict(next,statusLED.pin)){errorReply(409,"Status LED GPIO is reserved on this controller.");return;}
    if(physicalStop.enabled&&physicalStopConflict(next,physicalStopPin)){errorReply(409,"Emergency-stop pin is reserved on this controller. Choose another prop GPIO.");return;}

    if (!persistConfig(next)) { errorReply(507,"Flash save failed. Previous settings remain in use."); return; }

    // Stop old audio before changing the UART pins. Disarm has already stopped it.

    { EngineLock lock;

      for(unsigned k=0;k<2;k++)if(audioEnabled(k)&&audioRuntime[k].ready)sendAudio(0x16,0,k);

      releasePins(); config = next; setupPins(); configLoadFault=false;configureArmPin();configurePhysicalStopPin();samplePhysicalStop(millis());if(physicalStop.blocked())halt();

    }

    server.send(200,"application/json","{\"ok\":true}");

  });

  server.on("/api/status",HTTP_GET,[] {

    if (!authorized()) return;

    uint32_t now = millis(); JsonDocument doc;

    { EngineLock lock;

    samplePhysicalStop(now);doc["physicalStopEnabled"]=physicalStop.enabled;doc["physicalStopPin"]=physicalStopPin;doc["physicalStopLatched"]=physicalStop.latched;doc["physicalStopOpen"]=physicalStop.open;doc["physicalStopReady"]=physicalStop.ready(now);
    doc["statusLEDEnabled"]=statusLED.enabled;doc["statusLEDPin"]=statusLED.pin;doc["statusLEDPixels"]=statusLED.pixels;doc["statusLEDBrightness"]=statusLED.brightness;doc["statusLEDFault"]=statusLEDFault;doc["statusLEDColor"]=statusLEDColor();
    doc["armInputEnabled"]=armInput.enabled;doc["armInputPin"]=armInputPin;doc["sequenceTest"]=engine.previewing;doc["updating"]=otaBusy;doc["configRecovery"]=configLoadFault;doc["freeHeap"]=ESP.getFreeHeap();
    doc["setupComplete"]=prefs.getBool("setup",prefs.isKey("network")); doc["armed"] = engine.armed; doc["running"] = engine.running;doc["idle"]=engine.idle;doc["scene"]=engine.sceneIndex(); doc["elapsed"] = engine.elapsed(now);

    doc["cooldown"] = engine.cooldownRemaining(now); doc["trigger"] = engine.triggerActive(); doc["runs"] = engine.runCount;

    doc["wifiConnected"] = WiFi.status() == WL_CONNECTED; doc["stationIP"] = WiFi.localIP().toString();

    doc["deviceName"]=deviceName;doc["hostname"]=deviceHost;doc["ssid"] = wifiSSID; doc["apName"] = apName; doc["apIP"] = WiFi.softAPIP().toString();

    doc["servoAngle"]=servoCommandedAngle; doc["servoActive"]=servoAttached; doc["servoFault"]=servoFault; doc["stormFault"]=stormFaults[0]||stormFaults[1]; doc["light1"]=lightColors[0][0];doc["light2"]=lightColors[1][0];doc["stormFault1"]=stormFaults[0];doc["stormFault2"]=stormFaults[1]; doc["repeatRemaining"] = engine.repeatRemaining(now); doc["fsOK"] = fsOK; doc["version"] = buildVersion;

    doc["audio"] = (config.audio||config.audio2)?(((!config.audio||audioRuntime[0].ready)&&(!config.audio2||audioRuntime[1].ready))?(config.audio2?"Dual players ready · playback unverified":"Commands ready · playback unverified"):"Player initializing"):"Disabled";

    JsonArray states = doc["outputs"].to<JsonArray>(); for (size_t i=0;i<config.outputCount;++i) states.add(outputGuard.state[i]);

    } jsonReply(doc);

  });

  server.on("/api/arm",HTTP_POST,[] {

    if (!authorized()) return;

    { EngineLock lock; if(!physicalMotionAllowed())return; disableServo(); engine.arm(true,millis(),digitalRead(config.triggerPin)); writeOutputs(millis()); lastRun=false; ambient(); }

    server.send(200,"application/json","{\"ok\":true}");

  });

  server.on("/api/stop",HTTP_POST,[] { if (!authorized()) return; { EngineLock lock; halt(); } server.send(200,"application/json","{\"ok\":true}"); });

  server.on("/api/fire",HTTP_POST,[] {

    if (!authorized()) return;

    bool started; { EngineLock lock; if(!physicalMotionAllowed())return; started=engine.start(millis(),esp_random()); if (started) { writeOutputs(millis()); scare(); lastRun=true; } }

    if (!started) { errorReply(409,"Arm first; wait for the current run or cooldown to finish."); return; }

    server.send(200,"application/json","{\"ok\":true}");

  });

  server.on("/api/light-test",HTTP_POST,[]{if(!authorized())return;JsonDocument d;if(server.arg("plain").length()>200||deserializeJson(d,server.arg("plain"))||!number(d["channel"],0,1)||!d["active"].is<bool>()||(d["profile"]!="idle"&&d["profile"]!="event")){errorReply(400,"Invalid lighting test.");return;}unsigned k=d["channel"];{EngineLock lock;if(!stoppedForEdit())return;if(d["active"].as<bool>()&&!physicalMotionAllowed())return;if(!stormConfig(k).enabled||!stormAttached[k]){errorReply(409,"Enable and save this lighting channel first.");return;}lightTesting[k]=d["active"];lightTestEvent[k]=d["profile"]=="event";lightTestStart[k]=millis();lightWasArmed[k]=false;stormEngines[k].reset();writeOutputs(millis());}server.send(200,"application/json","{\"ok\":true}");});
  server.on("/api/test",HTTP_POST,[] {

    if (!authorized()) return;

    JsonDocument d; if (deserializeJson(d,server.arg("plain")) || !number(d["output"],0,config.outputCount-1)) { errorReply(400,"Invalid output."); return; }

    bool tested; { EngineLock lock; if (!stoppedForEdit()) return; if(!physicalMotionAllowed())return; tested=engine.test(d["output"],millis()); writeOutputs(millis()); }

    if (!tested) { errorReply(409,"Wait for the previous test pulse."); return; }

    server.send(200,"application/json","{\"ok\":true}");

  });

  server.on("/api/network",HTTP_POST,[] {

    if (!authorized()) return;

    { EngineLock lock; if (!stoppedForEdit()) return; halt(); }

    JsonDocument d; if (deserializeJson(d,server.arg("plain"))) { errorReply(400,"Invalid JSON."); return; }

    String ssid = d["ssid"] | ""; String pass = d["password"] | ""; String password = d["adminPassword"] | "";

    if (!d["ssid"].is<const char*>() || ssid.length()>32 || pass.length()>63 || (pass.length()>0 && pass.length()<8) || (password.length()>0 && (password.length()<8 || password.length()>63))) { errorReply(400,"Use an SSID up to 32 bytes and passwords of 8–63 characters (blank Wi-Fi password for an open network)."); return; }

    if (!nvsOK) { errorReply(507,"Network storage unavailable."); return; }

    String nextName=d["deviceName"] | deviceName;

    if (!nameField(d["deviceName"],32)) { errorReply(400,"Choose a controller name of 1–32 bytes."); return; }

    String nextPass = password.length() ? password : adminPassword;

    JsonDocument network; network["name"]=nextName;network["ssid"]=ssid; network["wifi"]=pass; network["admin"]=nextPass;

    String payload; serializeJson(network,payload);

    if (prefs.putString("network",payload)!=payload.length()) { errorReply(507,"Could not save network settings."); return; }

    server.send(200,"application/json","{\"ok\":true,\"reboot\":true}"); rebootAt=millis(); rebootPending=true;

  });

  server.onNotFound([] { if (authorized()) errorReply(404,"Not found."); });

}

void setup() {

  Serial.begin(115200); defaults();

  engineMutex=xSemaphoreCreateMutex();

  if (!engineMutex) { Serial.println("Controller mutex allocation failed."); while(true) delay(1000); }

  // First boot initializes the flash filesystem. Config always starts disarmed.

  fsOK = LittleFS.begin(false);
  if(!fsOK){
    const auto* partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_ANY,"spiffs");
    bool erased=partition;uint8_t buffer[256];
    for(size_t at=0;erased&&at<partition->size;at+=sizeof(buffer)){
      if(esp_partition_read(partition,at,buffer,sizeof(buffer))!=ESP_OK){erased=false;break;}
      for(auto byte:buffer)if(byte!=0xff){erased=false;break;}
    }
    if(erased&&LittleFS.format())fsOK=LittleFS.begin(false);
  }

  if (fsOK) {

    #if defined(CONFIG_IDF_TARGET_ESP32S2)
    std::unique_ptr<Config> savedMemory(new(std::nothrow) Config);if(!savedMemory){Serial.println("Config memory unavailable; halted.");while(true)delay(1000);}Config& saved=*savedMemory;
#else
    static Config saved;
#endif


    if (readConfig("/config.json",saved) || readConfig("/config.bak",saved)) config=saved;
    else if(LittleFS.exists("/config.json")||LittleFS.exists("/config.bak"))configLoadFault=true;

  }

  setupPins();

  nvsOK = prefs.begin("hauntcue",false);
  if(!nvsOK){halt();Serial.println("Safety settings unavailable. Controller halted.");while(true)delay(1000);}
  if(!physicalStopSelfTest()){halt();Serial.println("Physical stop self-test FAILED. Controller halted.");while(true)delay(1000);}
  Serial.println("Physical stop self-test PASS");
  JsonDocument safety;
  bool safetyEnabled=false;
  if(nvsOK&&prefs.isKey("physical-stop")){
   if(deserializeJson(safety,prefs.getString("physical-stop","{}"))||!safety["enabled"].is<bool>()||!number(safety["pin"],0,48)||!inputPin(safety["pin"])){
    halt();Serial.println("Invalid physical stop settings. Controller halted.");while(true)delay(1000);
   }
   safetyEnabled=safety["enabled"];physicalStopPin=safety["pin"];
  }
  physicalStop.configure(safetyEnabled,millis());configurePhysicalStopPin();samplePhysicalStop(millis());if(physicalStop.blocked())halt();
  if(prefs.isKey("status-led")){JsonDocument led;if(!deserializeJson(led,prefs.getString("status-led","{}"))&&led["enabled"].is<bool>()&&number(led["pin"],0,48)&&statusPinSupported(led["pin"])&&number(led["pixels"],1,8)&&number(led["brightness"],1,100)){statusLED.enabled=led["enabled"];statusLED.pin=led["pin"];statusLED.pixels=led["pixels"];statusLED.brightness=led["brightness"];setupStatusLED();}else{statusLEDFault=true;Serial.println("Invalid status LED settings; indicator disabled.");}}



  if(prefs.isKey("arm-input")){JsonDocument arm;if(!deserializeJson(arm,prefs.getString("arm-input","{}"))&&arm["enabled"].is<bool>()&&arm["toggle"].is<bool>()&&number(arm["pin"],-1,48)&&(!arm["enabled"].as<bool>()||armPinSupported(arm["pin"]))){armInputPin=arm["pin"];armInput.configure(arm["enabled"],arm["toggle"],millis(),false);configureArmPin();}}
  adminPassword="hauntcue123";

  if (nvsOK) {

    JsonDocument network;

    if (!deserializeJson(network,prefs.getString("network","{}"))) {

      String password=network["admin"] | "";

      if (password.length()>=8 && password.length()<=63) adminPassword=password;

      deviceName=network["name"] | "";wifiSSID=network["ssid"] | ""; wifiPassword=network["wifi"] | "";

    }

  }

  // eFuse MAC is little-endian: its low 24 bits are the shared vendor OUI.

  // Use the device-specific half so boards from the same vendor get distinct names.

  char suffix[7]; snprintf(suffix,sizeof(suffix),"%06X",unsigned((ESP.getEfuseMac() >> 24) & 0xFFFFFF));

  apName = String("HauntCue-")+suffix;

  deviceHost=String("hauntcue-")+suffix;deviceHost.toLowerCase();if(!deviceName.length())deviceName=apName;WiFi.setHostname(deviceHost.c_str()); WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(apName.c_str(),adminPassword.c_str()); WiFi.setAutoReconnect(true);

  if (wifiSSID.length()) WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());

  discoveryReady=MDNS.begin(deviceHost.c_str());

  if (discoveryReady) { MDNS.addService("http","tcp",80);MDNS.addService("hauntcue","tcp",80);MDNS.addServiceTxt("hauntcue","tcp","name",deviceName);MDNS.addServiceTxt("hauntcue","tcp","version",buildVersion); }

  const char* headers[]={"Content-Type","X-HauntCue-SHA256","X-HauntCue-Size","X-HauntCue-Target"}; server.collectHeaders(headers,4);

  initRoutes(); server.begin();

  if (xTaskCreatePinnedToCore([](void*) {

    for (;;) {

      { EngineLock lock;

        uint32_t now=millis();samplePhysicalStop(now);sampleArmPin(now);
        if(!physicalStop.blocked()&&!otaBusy&&!configLoadFault){engine.tick(now,digitalRead(config.triggerPin),esp_random());writeOutputs(now);servoTick(now);}

        if (!physicalStop.blocked()&&(engine.running!=lastRun || (engine.running&&audioProgram!=engine.generation()))) { if (engine.running) scare(); else if (engine.armed){ambient(!firstAudioTouched);firstAudioTouched=false;} else stopAudio(); lastRun=engine.running; }
        if(!physicalStop.blocked()&&lastIdle&&!engine.idle&&!engine.running&&engine.armed){ambient(!firstAudioTouched);firstAudioTouched=false;}lastIdle=engine.idle;if(!physicalStop.blocked())cueAudioTick(now);

        audioTick(now);statusLEDTick();

      }

      vTaskDelay(pdMS_TO_TICKS(2));

    }

  },"cue-engine",4096,nullptr,2,nullptr,(portNUM_PROCESSORS>1?1:0))!=pdPASS) {

    halt(); Serial.println("Cue task allocation failed. Controller halted."); while(true) delay(1000);

  }

  Serial.printf("\nHauntCue %s · DISARMED\nAP: %s\nOpen http://192.168.4.1 · user admin\n",buildVersion,apName.c_str());

  Serial.printf("Network hostname: %s.local\n",deviceHost.c_str());

}

void loop() {

  // Cue scheduling runs in its own task, independent of socket reads.

  server.handleClient();
  if(otaBusy&&!otaComplete&&uint32_t(millis()-otaActivity)>15000)abortFirmwareUpdate("Firmware upload timed out. Existing firmware and settings remain intact.");

  if (rebootPending && uint32_t(millis()-rebootAt)>=1000) { { EngineLock lock; halt(); } ESP.restart(); }

  delay(1);

}

