#include "IdleManager.h"
#include <esp_sleep.h>
void SleepManager::idleSleep(bool safe)  {
  if(!safe)return;
  esp_sleep_enable_timer_wakeup(10000);
  esp_light_sleep_start();
}
bool IdleManager::update(ServiceManager& s,InputEvent event)  {
  Key key=Keyboard::decode(s.input.raw);
  bool pressed=key!=Key::None&&key!=Key::Unknown;
  if(pressed||event!=InputEvent::None)  {
    lastActivity=millis();
    if(off)  {
      off=false;
      suppress=true;
      s.display.sleep(false);
      s.display.invalidate();
    }
  }
  if(suppress)  {
    if(key==Key::None&&event==InputEvent::None)suppress=false;
    return true;
  }
  if(s.config.values.timeout&&millis()-lastActivity>s.config.values.timeout*1000UL&&!off)  {
    off=true;
    s.display.sleep(s.config.values.screensaver==0);
    s.display.invalidate();
  }
  if(off&&s.config.values.screensaver)s.display.screensaver(s.config.values.screensaver,millis(),s.config.values.theme);
  if(off&&!s.config.values.screensaver&&millis()-lastActivity>300000)sleep.idleSleep(!s.wifi.enabled&&!s.ble.radioActive()&&!s.ir.busy&&!s.net.busy&&!s.download.active&&!s.nfc.busScanning&&!s.nfc.scanRequested&&!s.nfc.ndefBusy);
  return false;
}
