#pragma once
#include <Arduino.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
struct CaptureFrame  {
  uint32_t micros;
  uint16_t length,original;
  uint8_t bytes[256];
};
class CaptureService  {
  QueueHandle_t queue=nullptr;
  File file;
  uint32_t started=0;
  uint32_t originMicros=0;
  uint8_t filterBssid[6]=  {
  };
  static CaptureService* instance;
  static void receive(void* buf,wifi_promiscuous_pkt_type_t type);
  public:std::atomic<bool> active  {
    false
  };
  std::atomic<uint32_t> packets  {
    0
  },dropped  {
    0
  };
  String filename;
  bool start(const String& path,const String& bssid,uint8_t channel,bool writeFile);
  void stop();
  void update();
};
