#pragma once
// Detailed JSON field diagnostics run before conversion into fixed-size structs.
// Never clamp or truncate an imported/saved value to make it pass validation.
struct NumericLimit {const char* key;int lo,hi;bool optional=false;};
char validationPath[112]={},validationMessage[240]={};
const char* fieldProblem(const char* path,const char* message){
  strlcpy(validationPath,path,sizeof(validationPath));
  snprintf(validationMessage,sizeof(validationMessage),"%s: %s",path,message);
  return validationMessage;
}
const char* checkNumbers(JsonVariantConst object,const char* path,std::initializer_list<NumericLimit> limits){
  for(const auto& rule:limits){
    const auto value=object[rule.key];if(rule.optional&&value.isNull())continue;
    if(!value.is<int>()||value.as<int>()<rule.lo||value.as<int>()>rule.hi){
      char full[112],problem[128];String actual;serializeJson(value,actual);
      snprintf(full,sizeof(full),"%s%s%s",path,*path?".":"",rule.key);
      snprintf(problem,sizeof(problem),"must be an integer from %d to %d; received %.32s",rule.lo,rule.hi,actual.c_str());
      return fieldProblem(full,problem);
    }
  }
  return nullptr;
}
const char* validateConfigFields(JsonVariantConst root){
  validationPath[0]=validationMessage[0]=0;
  if(!root.is<JsonObjectConst>())return fieldProblem("config","expected a settings object");
  if(auto e=checkNumbers(root,"",{{"schema",1,1}}))return e;
  for(const char* light:{"storm","storm2"}){
    const auto st=root[light];if(st.isNull())continue;
    if(auto e=checkNumbers(st,light,{{"pin",0,48,true},{"channel",0,5},{"pixels",1,60},{"brightness",1,100},{"bursts",1,8},{"color",0,0xffffff},{"minWait",500,60000},{"maxWait",500,60000},{"flashMs",10,300},{"gapMs",20,500}}))return e;
    if(st["minWait"].as<int>()>st["maxWait"].as<int>())return fieldProblem(light,"minimum lightning quiet time must not exceed maximum quiet time");
    for(const char* profile:{"idle","event"}){
      if(st[profile].isNull()&&st["pin"].isNull())continue;
      char path[48];snprintf(path,sizeof(path),"%s.%s",light,profile);
      if(auto e=checkNumbers(st[profile],path,{{"effect",0,10},{"brightness",0,100},{"speed",100,10000},{"width",1,20},{"color",0,0xffffff},{"secondary",0,0xffffff}}))return e;
    }
  }
  if(auto e=checkNumbers(root["trigger"],"trigger",{{"pin",0,48},{"pull",0,2},{"debounce",5,1000},{"cooldown",0,60000}}))return e;
  if(auto e=checkNumbers(root["audio"],"audio",{{"tx",0,48},{"rx",0,48},{"volume",0,30},{"ambientTrack",0,2999}}))return e;
  if(!root["audio2"].isNull())if(auto e=checkNumbers(root["audio2"],"audio2",{{"tx",0,48},{"rx",0,48},{"volume",0,30}}))return e;
  if(!root["servo"].isNull())if(auto e=checkNumbers(root["servo"],"servo",{{"pin",0,48},{"minUs",500,2500},{"maxUs",500,2500},{"minAngle",0,180},{"maxAngle",0,180},{"home",0,180}}))return e;
  if(!root["repeat"].isNull())if(auto e=checkNumbers(root["repeat"],"repeat",{{"min",1000,3600000},{"max",1000,3600000}}))return e;
  if(!root["motion"].isNull())if(auto e=checkNumbers(root["motion"],"motion",{{"a",-1,5},{"b",-1,5},{"deadTime",20,2000}}))return e;
  if(!root["behavior"].isNull())if(auto e=checkNumbers(root["behavior"],"behavior",{{"idle",-1,3},{"idleGap",1000,3600000}}))return e;
  unsigned i=0;
  for(auto output:root["outputs"].as<JsonArrayConst>()){
    char path[48];snprintf(path,sizeof(path),"outputs.%u",i++);
    if(auto e=checkNumbers(output,path,{{"pin",0,48},{"maxOnMs",0,120000,true}}))return e;
  }
  i=0;
  for(auto scene:root["scenes"].as<JsonArrayConst>()){
    char path[72];snprintf(path,sizeof(path),"scenes.%u",i);
    if(auto e=checkNumbers(scene,path,{{"duration",1,120000},{"scareTrack",0,2999},{"enabledMask",0,2047,true}}))return e;
    if(scene["cues"].size()>haunt::MAX_CUES)return fieldProblem(path,"at most 128 output cues are allowed per scene");
    const int duration=scene["duration"];unsigned j=0;
    for(auto cue:scene["cues"].as<JsonArrayConst>()){
      snprintf(path,sizeof(path),"scenes.%u.cues.%u",i,j++);
      if(auto e=checkNumbers(cue,path,{{"output",0,int(root["outputs"].size())-1},{"start",0,duration-1},{"duration",1,duration-(cue["start"]|0)}}))return e;
    }
    j=0;
    for(auto cue:scene["servoMoves"].as<JsonArrayConst>()){
      snprintf(path,sizeof(path),"scenes.%u.servoMoves.%u",i,j++);
      if(auto e=checkNumbers(cue,path,{{"start",0,duration-1},{"duration",1,duration-(cue["start"]|0)},{"angle",root["servo"]["minAngle"]|20,root["servo"]["maxAngle"]|160}}))return e;
    }
    j=0;
    for(auto cue:scene["lightCues"].as<JsonArrayConst>()){
      snprintf(path,sizeof(path),"scenes.%u.lightCues.%u",i,j++);
      if(auto e=checkNumbers(cue,path,{{"channel",0,1},{"start",0,duration-1},{"duration",1,duration-(cue["start"]|0)}}))return e;
    }
    j=0;
    for(auto cue:scene["audioCues"].as<JsonArrayConst>()){
      snprintf(path,sizeof(path),"scenes.%u.audioCues.%u",i,j++);
      const int action=cue["action"]|0;
      if(auto e=checkNumbers(cue,path,{{"start",0,duration-1},{"player",0,1},{"action",0,2},{"value",action==0?1:0,action==0?2999:action==1?30:0}}))return e;
    }
    ++i;
  }
  return nullptr;
}
