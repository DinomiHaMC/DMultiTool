#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
struct IRSignal  {
  uint16_t address=0;
  uint8_t command=0x10,repeats=0;
};
struct IRJob  {
  IRSignal signal;
  uint16_t timings[256]=  {
  };
  uint16_t length=0;
  uint8_t khz=38;
};
class IRModule  {
  QueueHandle_t jobs=nullptr,done=nullptr;
  static void worker(void* arg);
  public: bool ready=false,busy=false;
  void begin();
  bool send(IRSignal signal);
  bool update();
  bool parse(const String& text,IRSignal& signal);
  bool sendRaw(const uint16_t* timings,size_t count,uint8_t khz=38);
};
