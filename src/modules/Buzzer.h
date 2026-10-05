#pragma once
#include <Arduino.h>
class Buzzer  {
  uint32_t started=0;
  uint16_t duration=0;
  public: void begin();
  void beep(bool enabled,uint16_t frequency=2200,uint16_t ms=35);
  void update();
};
