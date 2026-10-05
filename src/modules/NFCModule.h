#pragma once
#include <Arduino.h>
#include <atomic>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
struct I2CResult  {
  uint8_t addresses[126]=  {
  };
  size_t count=0;
};
struct NDEFJob  {
  uint32_t token=0;
  bool write=false,uri=false;
  char text[101]=  {
  };
};
struct NDEFResult  {
  uint32_t token=0;
  bool ok=false;
  char text[768]=  {
  };
};
struct TagInfo  {
  uint8_t uid[7]=  {
  }, length=0;
  uint16_t atqa=0;
  uint8_t sak=0;
  char hex[24]=  {
  };
};
class NFCModule  {
  Adafruit_PN532 reader  {
    255,255,&Wire
  };
  SemaphoreHandle_t bus=nullptr;
  QueueHandle_t tags=nullptr, busResults=nullptr, ndefJobs=nullptr, ndefResults=nullptr;
  TaskHandle_t task=nullptr;
  bool active=false;
  std::atomic<uint32_t> token  {
    0
  };
  static void worker(void* arg);
  bool getVersion(uint8_t* version);
  void ndefOperation(const NDEFJob& job,NDEFResult& result,const TagInfo& tag);
  public: bool available=false;
  TagInfo last;
  void begin();
  void scan(bool enabled);
  bool update();
  bool requestBusScan();
  bool updateBusScan(I2CResult& result);
  bool busScanning=false,ndefBusy=false,scanRequested=false;
  String ndefText;
  uint32_t ndefRevision=0;
  bool ndefOk=false;
  bool readNdef();
  bool writeNdef(const String& text,bool uri);
  bool updateNdef();
};
