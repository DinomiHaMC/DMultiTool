#include "ConfigStore.h"
void ConfigStore::begin()  {
  ready=nvs.begin("hp2000",false);
  if(!ready)return;
  apSSID=nvs.getString("apSSID","HP2000-Test");
  apPassword=nvs.getString("apPassword","");
  values.irCarrier=constrain(nvs.getUChar("irCarrier",38),30,60);
  values.rgbAddress=nvs.getUShort("rgbAddress",0xEF00);
  values.rotation=nvs.getUChar("rotation",0);
  values.theme=nvs.getUChar("theme",0);
  values.defaultApp=nvs.getUChar("defaultApp",0);
  values.logLevel=nvs.getUChar("logLevel",1);
  values.wifiChannel=nvs.getUChar("wifiChannel",1);
  values.sound=nvs.getBool("sound",true);
  values.uiBeep=nvs.getBool("beep",true);
  values.repeat=nvs.getBool("repeat",true);
  values.wifi=nvs.getBool("wifi",true);
  values.ble=nvs.getBool("ble",false);
  values.splash=nvs.getBool("splash",true);
  values.bootSound=nvs.getBool("bootSound",true);
  values.animations=nvs.getBool("animations",true);
  values.wrap=nvs.getBool("wrap",true);
  values.statusBar=nvs.getBool("statusBar",true);
  values.serialLog=nvs.getBool("serialLog",true);
  values.hardwareDebug=nvs.getBool("hardwareDebug",false);
  values.debugOverlay=nvs.getBool("debugOverlay",false);
  values.nfcSave=nvs.getBool("nfcSave",true);
  values.longPress=nvs.getUShort("long",800);
  values.timeout=nvs.getUShort("timeout",60);
  values.repeatDelay=nvs.getUShort("repeatDelay",450);
  values.repeatRate=nvs.getUShort("repeatRate",140);
  values.frequency=nvs.getUShort("frequency",2200);
  values.rotation%=4;
  values.theme%=4;
  values.logLevel%=4;
  values.longPress=constrain(values.longPress,400,2000);
  values.repeatDelay=constrain(values.repeatDelay,200,1500);
  values.repeatRate=constrain(values.repeatRate,60,500);
  values.timeout=constrain(values.timeout,0,600);
  values.frequency=constrain(values.frequency,500,5000);
  values.wifiChannel=constrain(values.wifiChannel,1,13);
  bootId=nvs.getUInt("boot",0)+1;
  nvs.putUInt("boot",bootId);
}
void ConfigStore::save()  {
  if(!ready)return;
  nvs.putString("apSSID",apSSID);
  nvs.putString("apPassword",apPassword);
  nvs.putUChar("irCarrier",values.irCarrier);
  nvs.putUShort("rgbAddress",values.rgbAddress);
  nvs.putUChar("rotation",values.rotation);
  nvs.putUChar("theme",values.theme);
  nvs.putUChar("defaultApp",values.defaultApp);
  nvs.putUChar("logLevel",values.logLevel);
  nvs.putUChar("wifiChannel",values.wifiChannel);
  nvs.putBool("sound",values.sound);
  nvs.putBool("beep",values.uiBeep);
  nvs.putBool("repeat",values.repeat);
  nvs.putBool("wifi",values.wifi);
  nvs.putBool("ble",values.ble);
  nvs.putBool("splash",values.splash);
  nvs.putBool("bootSound",values.bootSound);
  nvs.putBool("animations",values.animations);
  nvs.putBool("wrap",values.wrap);
  nvs.putBool("statusBar",values.statusBar);
  nvs.putBool("serialLog",values.serialLog);
  nvs.putBool("hardwareDebug",values.hardwareDebug);
  nvs.putBool("debugOverlay",values.debugOverlay);
  nvs.putBool("nfcSave",values.nfcSave);
  nvs.putUShort("long",values.longPress);
  nvs.putUShort("timeout",values.timeout);
  nvs.putUShort("repeatDelay",values.repeatDelay);
  nvs.putUShort("repeatRate",values.repeatRate);
  nvs.putUShort("frequency",values.frequency);
}
int ConfigStore::networkCount()  {
  return ready?min((int)nvs.getUChar("netCount",0),8):0;
}
SavedNetwork ConfigStore::network(int i)  {
  if(i<0||i>=networkCount())return  {
  };
  return  {
    nvs.getString(("ssid"+String(i)).c_str()),nvs.getString(("pass"+String(i)).c_str())
  };
}
bool ConfigStore::saveNetwork(const String& ssid,const String& password)  {
  if(!ready||ssid.isEmpty()||ssid.length()>32||password.length()>64)return false;
  int n=networkCount(),slot=n;
  for(int i=0;i<n;i++)if(network(i).ssid==ssid)  {
    slot=i;
    break;
  }
  if(slot>=8)return false;
  nvs.putString(("ssid"+String(slot)).c_str(),ssid);
  nvs.putString(("pass"+String(slot)).c_str(),password);
  if(slot==n)nvs.putUChar("netCount",n+1);
  return true;
}
void ConfigStore::forgetNetwork(int i)  {
  int n=networkCount();
  if(i<0||i>=n)return;
  for(int j=i;j<n-1;j++)  {
    auto next=network(j+1);
    nvs.putString(("ssid"+String(j)).c_str(),next.ssid);
    nvs.putString(("pass"+String(j)).c_str(),next.password);
  }
  nvs.remove(("ssid"+String(n-1)).c_str());
  nvs.remove(("pass"+String(n-1)).c_str());
  nvs.putUChar("netCount",n-1);
}
