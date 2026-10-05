#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../storage/ConfigStore.h"
class Logger  {
  QueueHandle_t queue=nullptr;
  Settings* settings=nullptr;
  uint32_t lastFlush=0;
  char buffer[2048]=  {
  };
  size_t used=0;
  public: static Logger* instance;
  void begin(Settings& s);
  void log(uint8_t level,const char* scope,const char* text);
  void update(bool sdReady);
};
