#pragma once
#include <Arduino.h>
class Buzzer  {
  uint32_t started=0;
  uint16_t duration=0;
  bool keyMode=false,keyDown=false;
  public: void begin();
  void beep(bool enabled,uint16_t frequency=2200,uint16_t ms=35);
  void beginKey();
  void key(bool down,uint16_t frequency=700);
  void endKey();
  void update();
};
