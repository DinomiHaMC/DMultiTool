#pragma once
#include "ServiceManager.h"
class BatteryManager  {
  public:enum class State  {
    Unavailable
  };
  State state()const  {
    return State::Unavailable;
  }
};
class SleepManager  {
  public:void idleSleep(bool safe);
};
class IdleManager  {
  uint32_t lastActivity=0;
  bool off=false,suppress=false;
  SleepManager sleep;
  public:void begin()  {
    lastActivity=millis();
  }
  bool update(ServiceManager& services,InputEvent event);
  void keepAwake() { lastActivity=millis(); }
  void wake(ServiceManager& services) { off=false;suppress=false;lastActivity=millis();services.display.sleep(false);services.display.invalidate(); }
  bool asleep()const  {
    return off;
  }
};
