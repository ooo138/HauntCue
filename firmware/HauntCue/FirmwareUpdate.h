#pragma once
#include <Update.h>
#include <esp_ota_ops.h>

const esp_partition_t* otaSlot(){
  const auto* running=esp_ota_get_running_partition();
  const auto* slot=esp_ota_get_next_update_partition(nullptr);
  return running&&slot&&slot->address!=running->address?slot:nullptr;
}
String otaError;
size_t otaExpected=0,otaReceived=0;
uint32_t otaActivity=0;
bool otaAccepted=false,otaComplete=false,otaResponseSent=false;

void updateReply(){
  JsonDocument d;const auto* slot=otaSlot();
  d["format"]="HauntCue application v1";d["version"]=buildVersion;d["family"]=BOARD_FAMILY;d["available"]=slot!=nullptr;
  d["maxBytes"]=slot?slot->size:0;d["busy"]=otaBusy;d["received"]=otaReceived;
  d["settingsPreserved"]=true;
  if(!slot)d["reason"]="This flash layout has no second application slot. A USB migration with a verified backup is required; Wi-Fi updates cannot repartition it.";
  jsonReply(d);
}
void abortFirmwareUpdate(const char* message){
  if(Update.isRunning())Update.abort();
  otaError=message;otaAccepted=false;otaComplete=false;
  EngineLock lock;otaBusy=false;halt();
}
void handleFirmwareUpload(){
  auto& upload=server.upload();
  if(upload.status==UPLOAD_FILE_START){
    otaError="";otaAccepted=otaComplete=otaResponseSent=false;otaReceived=0;
    if(!authorized(true)){otaResponseSent=true;otaError="Administrator login required.";return;}
    const String sha=server.header("X-HauntCue-SHA256");
    const String sizeHeader=server.header("X-HauntCue-Size");
    const auto* slot=otaSlot();
    if(!slot){otaError="No second OTA application slot. Existing settings were not changed.";return;}
    bool validSize=sizeHeader.length()>0&&sizeHeader.length()<9;
    for(unsigned i=0;i<sizeHeader.length();i++)validSize&=isDigit(sizeHeader[i]);
    otaExpected=sizeHeader.toInt();
    bool validHash=sha.length()==64;
    for(unsigned i=0;i<sha.length();i++)validHash&=isHexadecimalDigit(sha[i]);
    if(!validSize||!otaExpected||otaExpected>slot->size||!validHash||server.header("X-HauntCue-Target")!=BOARD_FAMILY){
      otaError="Choose a matching HauntCue application .bin within the displayed byte limit, with a SHA-256 checksum.";return;
    }
    {EngineLock lock;if(otaBusy||engine.armed||engine.running){otaError="Stop and disarm before updating.";return;}halt();otaBusy=true;}
    if(!Update.begin(otaExpected,U_FLASH)||!Update.setSHA256(sha.c_str())){abortFirmwareUpdate(Update.errorString());return;}
    otaAccepted=true;otaActivity=millis();
  }else if(upload.status==UPLOAD_FILE_WRITE&&otaAccepted){
    otaActivity=millis();
    if(upload.currentSize>otaExpected-otaReceived||Update.write(upload.buf,upload.currentSize)!=upload.currentSize){abortFirmwareUpdate("Firmware write failed or upload exceeds its declared size. Settings were not changed.");return;}
    otaReceived+=upload.currentSize;
  }else if(upload.status==UPLOAD_FILE_END&&otaAccepted){
    if(otaReceived!=otaExpected||!Update.end(false)){abortFirmwareUpdate("Firmware checksum, image, board or length verification failed. Current firmware and settings remain in place.");return;}
    otaAccepted=false;otaComplete=true;
  }else if(upload.status==UPLOAD_FILE_ABORTED&&otaBusy){
    abortFirmwareUpdate("Upload disconnected. Current firmware and settings remain in place.");
  }
}
void installUpdateRoutes(){
  server.on("/api/update",HTTP_GET,[]{if(authorized())updateReply();});
  server.on("/api/update",HTTP_POST,[]{
    if(otaResponseSent)return;
    if(!authorized(true))return;
    if(!otaComplete){errorReply(400,otaError.length()?otaError.c_str():"No complete firmware upload received.");return;}
    server.sendHeader("Connection","close");
    server.send(200,"application/json","{\"ok\":true,\"reboot\":true,\"settingsPreserved\":true}");
    rebootAt=millis();rebootPending=true;
  },handleFirmwareUpload);
}
