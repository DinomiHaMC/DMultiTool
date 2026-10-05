#pragma once
#include "Logger.h"
#include "../input/Keyboard.h"
#include "../ui/DisplayManager.h"
#include "../modules/SDModule.h"
#include "../modules/NFCModule.h"
#include "../modules/IRModule.h"
#include "../modules/WiFiModule.h"
#include "../modules/Buzzer.h"
#include "../services/BLEService.h"
#include "../services/NetworkTools.h"
#include "../services/CaptureService.h"
enum class ServiceState  {
  Available,Unavailable,Error
};
class ServiceManager  {
  uint32_t sequence=0;
  char serialBuffer[513]=  {
  };
  size_t serialUsed=0;
  bool wifiPausedForBLE=false;
  public:ConfigStore config;
  Logger logger;
  Keyboard input;
  DisplayManager display;
  SDModule sd;
  NFCModule nfc;
  IRModule ir;
  WiFiModule wifi;
  Buzzer buzzer;
  BLEUtilityService ble;
  NetworkTools net;
  CaptureService capture;
  uint32_t nfcRevision=0,wifiRevision=0;
  I2CResult busResult;
  bool tagFound=false,irSent=false,wifiChanged=false,bleChanged=false;
  String serialText()const  {
    return serialBuffer;
  }
  void begin();
  bool setBLEEnabled(bool on);
  bool wifiPaused()const { return wifiPausedForBLE; }
  void update();
  Status status();
  String uniquePath(const String& prefix,const String& ext);
  void beep(uint16_t frequency=0,uint16_t ms=35);
};
