#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
enum class NetOperation:uint8_t  {
  DNS,Ping,Hosts,Ports,TCP,Listen,HTTP
};
struct NetJob  {
  NetOperation operation;
  char host[192]=  {
  },text[512]=  {
  };
  uint16_t port=80,endPort=80;
};
struct NetResult  {
  char text[4096]=  {
  };
};
class NetworkTools  {
  QueueHandle_t results=nullptr;
  NetJob pending;
  std::atomic<bool> workerDone{false};
  static void worker(void* p);
  void execute(const NetJob& job,NetResult& result);
  public:bool ready=false,busy=false;
  std::atomic<bool> cancelled  {
    false
  };
  std::atomic<uint16_t> progress  {
    0
  };
  String output;
  uint32_t revision=0;
  void begin();
  bool start(const NetJob& job);
  void cancel()  {
    cancelled=true;
  }
  bool update();
};
