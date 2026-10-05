#include "ServiceManager.h"
#include "../pins.h"
#include "../config.h"
#include "../services/RadioMemoryPolicy.h"
#include <SPI.h>
#include <time.h>
void ServiceManager::begin()  {
  Serial.begin(115200);
  config.begin();
  logger.begin(config.values);
  pinMode(Pins::TFT_CS,OUTPUT);
  pinMode(Pins::SD_CS,OUTPUT);
  digitalWrite(Pins::TFT_CS,HIGH);
  digitalWrite(Pins::SD_CS,HIGH);
  SPI.begin(Pins::SCK,Pins::MISO,Pins::MOSI);
  display.begin(config.values.rotation);
  if(config.values.splash)display.splash();
  input.begin();
  buzzer.begin();
  LOG_INFO("BOOT","TFT OK");
  display.bootStatus("TFT OK | Keyboard ready");
  sd.begin();
  display.bootStatus(sd.mounted?"TFT OK | SD OK":"TFT OK | SD FAIL");
  nfc.begin();
  display.bootStatus(nfc.available?"NFC OK | IR starting":"NFC FAIL | IR starting");
  ir.begin();
  wifi.begin(config.values.wifi);
  net.begin();
  if(config.values.ble&&!setBLEEnabled(true))  {
    LOG_WARN("BLE","%s",ble.lastError().c_str());
  }
  display.bootStatus(String(ir.ready?"IR OK":"IR FAIL")+" | "+(wifi.enabled?"WiFi ON":"WiFi OFF")+" | "+(ble.enabled?"BLE ON":"BLE OFF"));
  if(config.values.bootSound)beep(2600,90);
}
bool ServiceManager::setBLEEnabled(bool on) {
  const bool wasPaused=wifiPausedForBLE;
  const bool mayPause=!net.busy&&!capture.active&&(WiFi.getMode()&WIFI_MODE_AP)==0;
  bool ok=RadioMemoryPolicy::setBLE(ble,wifi,on,mayPause,config.values.wifi,wifiPausedForBLE);
  if(!wasPaused&&wifiPausedForBLE)LOG_INFO("BLE","WiFi temporarily OFF to free RAM");
  return ok;
}
void ServiceManager::beep(uint16_t f,uint16_t ms)  {
  buzzer.beep(config.values.sound,f?f:config.values.frequency,ms);
}
void ServiceManager::update()  {
  for(int i=0;i<32&&Serial.available();i++)  {
    char c=Serial.read();
    if(c=='\r')continue;
    if(c!='\n'&&(c<32||c>126))c='.';
    if(serialUsed==512)  {
      memmove(serialBuffer,serialBuffer+128,384);
      serialUsed=384;
    }
    serialBuffer[serialUsed++]=c;
    serialBuffer[serialUsed]=0;
  }
  tagFound=nfc.update();
  if(tagFound)  {
    nfcRevision++;
    beep(2800,80);
  }
  nfc.updateNdef();
  if(nfc.updateBusScan(busResult))nfcRevision++;
  wifiChanged=wifi.update();
  if(wifiChanged)wifiRevision++;
  bleChanged=ble.update();
  RadioMemoryPolicy::restore(ble,wifi,config.values.wifi,wifiPausedForBLE);
  irSent=ir.update();
  net.update();
  capture.update();
  buzzer.update();
  logger.update(sd.mounted);
}
Status ServiceManager::status() {
  Status status {
    sd.mounted,wifi.enabled,WiFi.status()==WL_CONNECTED,ble.enabled,nfc.available,ESP.getFreeHeap(),millis()/1000
  };
  status.bleConnected=ble.connected();
  return status;
}
String ServiceManager::uniquePath(const String& prefix,const String& ext)  {
  time_t now=time(nullptr);
  char buf[128];
  if(now>1700000000)  {
    struct tm t;
    localtime_r(&now,&t);
    char stamp[96];
    snprintf(stamp,sizeof(stamp),"%04d%02d%02d_%02d%02d%02d",t.tm_year+1900,t.tm_mon+1,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);
    snprintf(buf,sizeof(buf),"%s_%s_%lu%s",prefix.c_str(),stamp,(unsigned long)++sequence,ext.c_str());
  }
  else snprintf(buf,sizeof(buf),"%s_b%lu_%lu_%lu%s",prefix.c_str(),(unsigned long)config.bootId,(unsigned long)millis(),(unsigned long)++sequence,ext.c_str());
  return buf;
}
