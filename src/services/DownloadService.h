#pragma once
#include <Arduino.h>
#include <SD.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
class DownloadService {
  struct Chunk { uint16_t size;uint8_t data[512]; };
  QueueHandle_t chunks=nullptr;
  File file;
  String url,target,temp,ca;
  std::atomic<bool> cancelled{false},workerDone{false};
  std::atomic<int> code{0};
  static void worker(void* context);
  void cleanup();
public:
  static constexpr uint32_t MaxSize=16*1024*1024;
  bool active=false;
  String message;
  uint32_t received=0,revision=0;
  bool start(const String& url,const String& path);
  void cancel() { cancelled=true; }
  bool update();
};
