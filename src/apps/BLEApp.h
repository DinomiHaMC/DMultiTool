#pragma once
#include "MenuApp.h"
class BLEApp:public MenuApp  {
  bool scanWaiting=false;
  uint32_t scanVersion=0;
  void home()override;
  void scan();
  void devices();
  void info(int i);
  void advertiser();
  void keyboard();
  void mouse();
  bool ensure();
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "Bluetooth";
  }
  Icon icon()const override  {
    return Icon::BLE;
  }
  void update()override;
  void onClose()override  {
    s.ble.cancelTyping();
    if(s.ble.scanning)s.ble.stop();
    scanWaiting=false;
  }
};
