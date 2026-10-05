#pragma once
#include <Arduino.h>
#include <atomic>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "BridgeChannel.h"
struct BLEInfo  {
  String name,address,services,manufacturer;
  int rssi=0;
};
class BLEUtilityService:public BridgeChannel  {
  class BridgeCallbacks:public NimBLECharacteristicCallbacks {
    BLEUtilityService& owner;
  public:
    explicit BridgeCallbacks(BLEUtilityService& service):owner(service) {}
    void onWrite(NimBLECharacteristic* characteristic,NimBLEConnInfo& info)override;
  } bridgeCallbacks{*this};
  QueueHandle_t frames=nullptr;
  NimBLECharacteristic* bridgeTX=nullptr;
  NimBLEScan* scanner=nullptr;
  NimBLEServer* server=nullptr;
  NimBLEHIDDevice* hid=nullptr;
  NimBLECharacteristic *keyboard=nullptr,*mouse=nullptr,*media=nullptr;
  bool initialized=false,releasePending=false;
  uint32_t keyTime=0,bridgeRetry=0;
  String typing;
  String error;
  size_t typed=0;
  static void scanWorker(void* p);
  bool initialize();
  void shutdown();
  public:bool enabled=false,advertising=false;
  std::atomic<bool> scanning  {
    false
  },scanDone  {
    false
  };
  BLEInfo devices[20];
  int count=0;
  uint32_t revision=0;
  bool setEnabled(bool on);
  static bool initializationMemoryLow();
  const String& lastError()const { return error; }
  bool radioActive()const { return initialized; }
  bool scan();
  bool update();
  bool advertise(const String& name,const String& manufacturer);
  void stop();
  bool startHID(bool reconnect=false);
  bool startBridge() { return startHID(true); }
  void keepBridgeAvailable() {
    if(millis()-bridgeRetry<1000)return;
    bridgeRetry=millis();
    if(enabled&&!scanning&&(!server||(!connected()&&!NimBLEDevice::getAdvertising()->isAdvertising())))startBridge();
  }
  bool receive(BLEFrame& frame) { return frames&&xQueueReceive(frames,&frame,0)==pdTRUE; }
  bool transmit(const uint8_t* bytes,size_t size);
  bool connected()const;
  bool typeText(const String& text);
  bool key(uint8_t usage,uint8_t modifier=0);
  bool consumer(uint16_t usage);
  bool move(int8_t x,int8_t y,int8_t wheel,uint8_t buttons=0);
  void cancelTyping()  {
    typing="";
  }
};
