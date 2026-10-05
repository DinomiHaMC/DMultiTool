#pragma once
#include <Arduino.h>
#include <Preferences.h>
struct Settings  {
  uint8_t irCarrier=38;
  uint16_t rgbAddress=0xEF00;
  uint8_t rotation=0, theme=0, defaultApp=0, logLevel=1, wifiChannel=1;
  bool sound=true, uiBeep=true, repeat=true, wifi=true, ble=false;
  bool splash=true, bootSound=true, animations=true, wrap=true, statusBar=true;
  bool serialLog=true, hardwareDebug=false, debugOverlay=false, nfcSave=true;
  uint16_t longPress=800, timeout=60, repeatDelay=450, repeatRate=140, frequency=2200;
  uint8_t screensaver=0;
  uint16_t customColors[7]={0x0841,0x18C3,0xFFFF,0x9CF3,0x07FF,0x224B,0xF800};
  bool notifyGlobal=false,notifyWake=false,notifyReceive=false;
};
struct SavedNetwork  {
  String ssid,password;
};
class ConfigStore  {
  Preferences nvs;
  bool ready=false;
  public: String apSSID="DMultiTool",apPassword="";
  Settings values;
  uint32_t bootId=0;
  void begin();
  void save();
  int networkCount();
  SavedNetwork network(int i);
  bool saveNetwork(const String& ssid,const String& password);
  void forgetNetwork(int i);
};
