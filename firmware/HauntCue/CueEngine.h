#pragma once
#include <stdint.h>
#include <stddef.h>
#include <initializer_list>
#include "StormEngine.h"
#include "BoardProfile.h"

namespace haunt {
constexpr size_t MAX_OUTPUTS = 6;
constexpr size_t MAX_SCENES = 4;
constexpr size_t MAX_CUES = 128;
constexpr size_t MAX_SERVO_MOVES = 32;
constexpr size_t MAX_CONFIG_BYTES = 65536;
constexpr uint32_t MAX_DURATION = 120000;
struct Output { char name[25] = ""; int8_t pin = -1; bool activeHigh = true; uint32_t maxOnMs = 0; };
struct Cue { uint8_t output = 0; uint32_t start = 0, duration = 1000; };
struct AudioCue { uint32_t start=0; uint8_t player=0,action=0; uint16_t value=1; };
struct ServoMove { uint32_t start = 0, duration = 500; uint8_t angle = 90; bool ease=false; };
struct Scene {
  char name[33] = "";
  uint32_t duration = 10000;
  uint16_t scareTrack = 2;
  uint16_t enabledMask = 2047;
  uint8_t count = 0;
  Cue cues[MAX_CUES];
  uint8_t audioCount=0;AudioCue audioCues[8];
  uint8_t lightCount=0;LightCue lightCues[16];
  bool randomServo=false;
  uint8_t servoCount = 0;
  ServoMove servoMoves[MAX_SERVO_MOVES];
};
struct Config {
  StormConfig storm=[] {StormConfig s;s.pin=BOARD_LIGHT1;return s;}();
  StormConfig storm2 = []{StormConfig s;s.channel=4;s.pin=BOARD_LIGHT2;s.minWait=5000;s.maxWait=16000;return s;}();
  Output outputs[MAX_OUTPUTS];
  uint8_t outputCount = BOARD_OUTPUTS, sceneCount = 1, selected = 0;
  Scene scenes[MAX_SCENES];
  int8_t triggerPin = BOARD_TRIGGER;
  bool triggerHigh = false, triggerEnabled = true;
  uint8_t pull = 1; // 0 external, 1 up, 2 down
  uint32_t debounce = 40, cooldown = 3000;
  bool audio = false;
  bool audioDY = false;
  bool audio2=false,audio2DY=false;int8_t audio2Tx=BOARD_TX2,audio2Rx=BOARD_RX2;uint8_t volume2=20;
  int8_t audioTx = BOARD_TX, audioRx = BOARD_RX;
  uint8_t volume = 20;
  uint16_t ambientTrack = 1;
  bool randomScenes=false;int8_t idleScene=-1;uint32_t idleGap=10000;
  bool repeat = false;
  uint32_t repeatMin = 45000, repeatMax = 90000;
  int8_t interlockA = -1, interlockB = -1;
  uint32_t deadTime = 200;
  bool pneumaticInterlock = false;
  bool servo = false;
  int8_t servoPin = BOARD_SERVO;
  uint16_t servoMinUs = 1000, servoMaxUs = 2000;
  uint8_t servoMinAngle = 20, servoMaxAngle = 160, servoHome = 90;
};
inline bool outputPin(int p){return boardOutputPin(p);}
inline bool inputPin(int p){return boardInputPin(p);}
inline const char* validate(const Config& c) {
  if(c.audio2&&BOARD_AUDIO<2)return "This board supports one serial audio player.";
  for(const auto* st:{&c.storm,&c.storm2}){
   if(!outputPin(st->pin)||!validProfile(st->idle,st->addressable)||!validProfile(st->event,st->addressable))return "Invalid lighting pin or effect.";
   if(const char* e=validateStorm(*st,6,-1,-1,true))return e;
   if(st->enabled){if(st->pin==c.triggerPin||(c.servo&&st->pin==c.servoPin)||(c.audio&&(st->pin==c.audioTx||st->pin==c.audioRx)))return "Lighting GPIO conflicts with trigger, servo or audio.";for(unsigned i=0;i<c.outputCount;i++)if(st->pin==c.outputs[i].pin)return "Lighting needs a dedicated GPIO.";}
  }
  if(c.storm.enabled&&c.storm2.enabled&&c.storm.pin==c.storm2.pin)return "Lighting pins must differ.";
  if (c.repeatMin < 1000 || c.repeatMax > 3600000 || c.repeatMin > c.repeatMax) return "Repeat interval must be 1–3600 seconds.";
  if (c.deadTime < 20 || c.deadTime > 2000) return "Direction dead time must be 20–2000 ms.";
  if (!(c.interlockA == -1 && c.interlockB == -1) && (c.interlockA < 0 || c.interlockB < 0 || c.interlockA == c.interlockB || c.interlockA >= c.outputCount || c.interlockB >= c.outputCount)) return "Choose two different interlocked outputs, or disable both.";
  if (!c.pneumaticInterlock && c.interlockA >= 0 && (!c.outputs[c.interlockA].activeHigh || !c.outputs[c.interlockB].activeHigh)) return "Interlocked H-bridge inputs require Active HIGH.";
  if (!c.outputCount || c.outputCount > BOARD_OUTPUTS) return "Choose 1–6 outputs.";
  if (!c.sceneCount || c.sceneCount > MAX_SCENES || c.selected >= c.sceneCount) return "Invalid scene selection.";
  if (!inputPin(c.triggerPin)) return "Trigger pin is not supported on classic ESP32.";
  if (!outputPin(c.servoPin) || c.servoMinUs < 500 || c.servoMaxUs > 2500 || c.servoMinUs >= c.servoMaxUs || c.servoMaxAngle > 180 || c.servoMinAngle >= c.servoMaxAngle || c.servoHome < c.servoMinAngle || c.servoHome > c.servoMaxAngle) return "Invalid servo travel or pulse range.";
  if (c.servo && (c.servoPin == c.triggerPin || (c.audio && (c.servoPin == c.audioTx || c.servoPin == c.audioRx)))) return "Servo GPIO conflicts with trigger or audio.";
  if (c.pull > 2 || (boardNeedsExternalPull(c.triggerPin) && c.pull != 0)) return "GPIO34–39 require an external pull resistor.";
  if (c.debounce < 5 || c.debounce > 1000 || c.cooldown > 60000) return "Invalid trigger timing.";
  for (size_t i = 0; i < c.outputCount; ++i) {
    if (c.outputs[i].maxOnMs > MAX_DURATION) return "Output pulse limit exceeds 120 seconds.";
    if (!outputPin(c.outputs[i].pin)) return "Unsupported output pin.";
    if (c.servo && c.servoPin == c.outputs[i].pin) return "Servo GPIO conflicts with an output.";
    if (c.outputs[i].pin == c.triggerPin) return "Trigger and output pins must differ.";
    for (size_t j = 0; j < i; ++j) if (c.outputs[i].pin == c.outputs[j].pin) return "Output pins must be unique.";
  }
  if (c.audio) {
    if (!outputPin(c.audioTx) || !inputPin(c.audioRx) || c.audioTx == c.audioRx) return "Invalid DFPlayer UART pins.";
    if (c.audioTx == c.triggerPin || c.audioRx == c.triggerPin) return "DFPlayer pins conflict with trigger.";
    for (size_t i = 0; i < c.outputCount; ++i)
      if (c.outputs[i].pin == c.audioTx || c.outputs[i].pin == c.audioRx) return "DFPlayer pins conflict with output.";
  }
  if(c.volume2>30)return "Invalid player 2 volume.";
  if(c.audio2){
   if(!outputPin(c.audio2Tx)||!inputPin(c.audio2Rx)||c.audio2Tx==c.audio2Rx)return "Invalid player 2 UART pins.";
   for(int pin:{int(c.audio2Tx),int(c.audio2Rx)}){
    if(pin==c.triggerPin||(c.servo&&pin==c.servoPin)||(c.audio&&(pin==c.audioTx||pin==c.audioRx))||(c.storm.enabled&&pin==c.storm.pin)||(c.storm2.enabled&&pin==c.storm2.pin))return "Player 2 pin conflicts with configured hardware.";
    for(unsigned i=0;i<c.outputCount;i++)if(pin==c.outputs[i].pin)return "Player 2 pin conflicts with output.";
   }
  }
  if (c.volume > 30 || c.ambientTrack > 2999) return "Invalid volume or ambient track.";
  if(c.idleScene < -1 || c.idleScene>=c.sceneCount || c.idleGap<1000 || c.idleGap>3600000 || (c.idleScene>=0 && c.idleScene==c.selected))return "Idle must use a separate scene and a 1-3600 second gap.";
  for (size_t s = 0; s < c.sceneCount; ++s) {
    const Scene& scene = c.scenes[s];
    if(scene.audioCount>8)return "Use at most eight audio cues.";
    for(unsigned i=0;i<scene.audioCount;i++){const auto& q=scene.audioCues[i];if(q.start>=scene.duration||q.player>1||q.action>2||(q.action==0&&(!q.value||q.value>2999))||(q.action==1&&q.value>30)||(q.action==2&&q.value!=0))return "Invalid audio cue.";for(unsigned j=0;j<i;j++){const auto& z=scene.audioCues[j];uint32_t gap=q.start>z.start?q.start-z.start:z.start-q.start;if(q.player==z.player&&gap<500)return "Audio cues on one player need 500ms separation.";}}
    if(scene.lightCount>16)return "Use at most 16 lighting cues.";
    for(unsigned i=0;i<scene.lightCount;i++){const auto& q=scene.lightCues[i];if(q.channel>1||!q.duration||q.start>=scene.duration||q.duration>scene.duration-q.start)return "Lighting cue must fit scene.";for(unsigned j=0;j<i;j++){const auto& z=scene.lightCues[j];if(q.channel==z.channel&&q.start<z.start+z.duration&&z.start<q.start+q.duration)return "Lighting cues cannot overlap on one channel.";}}
    if (scene.servoCount > MAX_SERVO_MOVES) return "Use at most 32 servo moves per scene.";
    for (size_t i=0;i<scene.servoCount;++i) {
      const ServoMove& m=scene.servoMoves[i];
      if (!m.duration || m.start >= scene.duration || m.duration > scene.duration-m.start || m.angle < c.servoMinAngle || m.angle > c.servoMaxAngle) return "Servo movements must fit the scene and travel limits.";
      for (size_t j=0;j<i;++j) { const ServoMove& other=scene.servoMoves[j]; if (m.start < other.start+other.duration && other.start < m.start+m.duration) return "Servo movement blocks cannot overlap."; }
    }
    if (!scene.duration || scene.duration > MAX_DURATION || scene.count > MAX_CUES || scene.scareTrack > 2999) return "Invalid scene duration, track, or cue count.";
    for (size_t i = 0; i < scene.count; ++i) {
      const Cue& cue = scene.cues[i];
      if (cue.output >= c.outputCount || !cue.duration || cue.start >= scene.duration || cue.duration > scene.duration - cue.start) return "Cues must fit inside the scene and reference an existing output.";
      if (c.outputs[cue.output].maxOnMs && cue.duration > c.outputs[cue.output].maxOnMs) return "Cue exceeds this output's pulse limit.";
      for (size_t j = 0; j < i; ++j) {
        const Cue& other = scene.cues[j];
        bool opposed = c.interlockA >= 0 && ((cue.output == c.interlockA && other.output == c.interlockB) || (cue.output == c.interlockB && other.output == c.interlockA));
        if (opposed && cue.start < other.start + other.duration + c.deadTime && other.start < cue.start + cue.duration + c.deadTime) return "Sway directions need an inactive gap equal to the dead time.";
      }
    }
  }
  return nullptr;
}

// Commanded angle; no position feedback. Hold the last target between moves.
inline uint8_t servoAngle(const Scene& scene, uint32_t t, uint8_t home,uint32_t seed=1) {
  uint8_t low=180,high=0;for(size_t i=0;i<scene.servoCount;i++){if(scene.servoMoves[i].angle<low)low=scene.servoMoves[i].angle;if(scene.servoMoves[i].angle>high)high=scene.servoMoves[i].angle;}
  auto target=[&](const ServoMove& m){if(!scene.randomServo)return m.angle;uint32_t x=seed^uint32_t((&m-scene.servoMoves+1)*2654435761UL);x^=x>>16;x*=2246822519UL;x^=x>>13;return uint8_t(low+x%(high-low+1));};
  uint8_t from=home; uint32_t lastEnd=0; const ServoMove* active=nullptr;
  for (size_t i=0;i<scene.servoCount;++i) {
    const ServoMove& m=scene.servoMoves[i];
    if (m.start+m.duration <= t && m.start+m.duration >= lastEnd) { from=target(m); lastEnd=m.start+m.duration; }
    if (t>=m.start && t-m.start<m.duration) active=&m;
  }
  if (!active) return from;
  float u=float(t-active->start)/active->duration;if(active->ease)u=u*u*(3-2*u);return uint8_t(int(from)+(int(target(*active))-int(from))*u);
}

// Chronological, once-only commands. A full serial queue defers, rather than drops, a cue.
class AudioCueClock {
public:
  void reset(){done=0;}
  template<class Emit> void tick(const Scene& scene,uint32_t elapsed,Emit emit){for(unsigned n=0;n<scene.audioCount;n++){int next=-1;for(unsigned i=0;i<scene.audioCount;i++)if(!(done&(1u<<i))&&scene.audioCues[i].start<=elapsed&&(next<0||scene.audioCues[i].start<scene.audioCues[next].start))next=i;if(next<0||!emit(scene.audioCues[next]))return;done|=1u<<next;}}
private:uint8_t done=0;
};
// No networking, delays, allocations, or Arduino dependencies in the scheduler.
class Engine {
public:
  bool armed = false, running = false, idle=false, previewing=false;
  uint32_t runCount = 0;
  void configure(const Config* next, uint32_t now, bool raw) {
    cfg = next; armed = running = idle = testing = cooling = repeating = previewing = false;activeIndex=cfg->selected;lastScene=255;quietAt=now;serial=0;
    candidate = stable = raw; changedAt = now; releaseSeen = raw != cfg->triggerHigh;
  }
  void arm(bool value, uint32_t now, bool raw) {
    stop(now); armed = value; candidate = stable = raw; changedAt = now;
    releaseSeen = raw != cfg->triggerHigh;quietAt=now;
  }
  bool start(uint32_t now,uint32_t random=0) {
    if (!armed || running || testing || cooldownRemaining(now)) return false;
    activeIndex=cfg->selected;
    if(cfg->randomScenes){uint8_t choices[MAX_SCENES],count=0;for(uint8_t i=0;i<cfg->sceneCount;i++)if(i!=cfg->idleScene)choices[count++]=i;if(count>1){uint8_t filtered[MAX_SCENES],n=0;for(unsigned i=0;i<count;i++)if(choices[i]!=lastScene)filtered[n++]=choices[i];activeIndex=filtered[random%n];}else activeIndex=choices[0];}
    lastScene=activeIndex;idle=false;running = true; repeating = false; startedAt = now; ++runCount;++serial;return true;
  }
  void stop(uint32_t now) {
    const bool hadRun = running;
    running = idle = testing = repeating = previewing = false;quietAt=now;
    if (hadRun) { finishedAt = now; cooling = true; }
  }
  bool preview(uint8_t index,uint32_t now) {
    if(armed||running||testing||index>=cfg->sceneCount)return false;
    activeIndex=index;idle=false;running=true;previewing=true;repeating=false;startedAt=now;++serial;return true;
  }
  bool test(uint8_t index, uint32_t now) {
    if (armed || running || testing || index >= cfg->outputCount) return false;
    testing = true; testIndex = index; testStarted = now; return true;
  }
  void tick(uint32_t now, bool raw, uint32_t random = 0) {
    if (cooling && uint32_t(now - finishedAt) >= cfg->cooldown) cooling = false;
    if (testing && uint32_t(now - testStarted) >= 500) testing = false;
    if (running && elapsed(now) >= cfg->scenes[activeIndex].duration) stop(now);
    if(idle && elapsed(now)>=cfg->scenes[activeIndex].duration){idle=false;quietAt=now;}
    if (raw != candidate) { candidate = raw; changedAt = now; }
    if (candidate != stable && uint32_t(now - changedAt) >= cfg->debounce) {
      stable = candidate;
      if (stable != cfg->triggerHigh) releaseSeen = true;
      else if (releaseSeen && cfg->triggerEnabled) { releaseSeen = false; start(now,random); }
    }
    if (armed && cfg->repeat && !running && !testing) {
      if (!repeating) { repeating = true; repeatAt = now; repeatDelay = cfg->repeatMin + random % (cfg->repeatMax - cfg->repeatMin + 1); }
      if (uint32_t(now - repeatAt) >= repeatDelay) start(now,random);
    } else repeating = false;
    if(armed&&!running&&!idle&&!testing&&cfg->idleScene>=0&&!cooldownRemaining(now)&&uint32_t(now-quietAt)>=cfg->idleGap){idle=true;activeIndex=cfg->idleScene;startedAt=now;++serial;}
  }
  bool on(size_t index, uint32_t now) const {
    if (testing) return index == testIndex && uint32_t(now - testStarted) < 500;
    if (!running&&!idle) return false;
    const uint32_t t = elapsed(now);
    const Scene& scene = cfg->scenes[activeIndex];
    if (t >= scene.duration) return false;
    // Overlapping cues on a channel merge; one ending cannot cancel another.
    for (size_t i = 0; i < scene.count; ++i) {
      const Cue& cue = scene.cues[i];
      if ((scene.enabledMask & (1u<<index)) && cue.output == index && t >= cue.start && t - cue.start < cue.duration) return true;
    }
    return false;
  }
  uint32_t elapsed(uint32_t now) const { return (running||idle) ? uint32_t(now - startedAt) : 0; }
  uint32_t cooldownRemaining(uint32_t now) const {
    if (!cooling || uint32_t(now - finishedAt) >= cfg->cooldown) return 0;
    return cfg->cooldown - uint32_t(now - finishedAt);
  }
  uint8_t sceneIndex() const {return activeIndex;}
  uint32_t generation() const {return serial;}
  bool triggerActive() const { return cfg->triggerEnabled && stable == cfg->triggerHigh; }
  uint32_t repeatRemaining(uint32_t now) const { return repeating && uint32_t(now-repeatAt) < repeatDelay ? repeatDelay-uint32_t(now-repeatAt) : 0; }
private:
  const Config* cfg = nullptr;
  uint8_t activeIndex=0,lastScene=255;uint32_t quietAt=0,serial=0;
  bool candidate = false, stable = false, releaseSeen = false, testing = false, cooling = false;
  uint8_t testIndex = 0;
  uint32_t changedAt = 0, startedAt = 0, finishedAt = 0, testStarted = 0;
  bool repeating = false;
  uint32_t repeatAt = 0, repeatDelay = 0;
};

// Applies to cue playback AND manual tests. Continuous requests cannot bypass a pulse limit.
class OutputGuard {
public:
  bool state[MAX_OUTPUTS] = {};
  void reset() { for (size_t i=0;i<MAX_OUTPUTS;++i) { state[i]=blocked[i]=wasOff[i]=false; } }
  void tick(const Config& c, const bool* requested, uint32_t now) {
    bool next[MAX_OUTPUTS] = {};
    for (size_t i=0;i<c.outputCount;++i) {
      if (!requested[i]) blocked[i]=false;
      if (state[i] && c.outputs[i].maxOnMs && uint32_t(now-started[i]) >= c.outputs[i].maxOnMs) blocked[i]=true;
      next[i]=requested[i] && !blocked[i];
    }
    if (c.interlockA>=0) {
      size_t a=c.interlockA,b=c.interlockB;
      if (requested[a] && requested[b]) next[a]=next[b]=false;
      if (next[a] && (state[b] || (wasOff[b] && uint32_t(now-offAt[b]) < c.deadTime))) next[a]=false;
      if (next[b] && (state[a] || (wasOff[a] && uint32_t(now-offAt[a]) < c.deadTime))) next[b]=false;
    }
    for (size_t i=0;i<c.outputCount;++i) {
      if (state[i] && !next[i]) { offAt[i]=now; wasOff[i]=true; }
      if (!state[i] && next[i]) started[i]=now;
      state[i]=next[i];
    }
  }
private:
  bool blocked[MAX_OUTPUTS] = {}, wasOff[MAX_OUTPUTS] = {};
  uint32_t started[MAX_OUTPUTS] = {}, offAt[MAX_OUTPUTS] = {};
};
}
