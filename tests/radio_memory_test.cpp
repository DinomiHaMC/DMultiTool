#include "../src/services/RadioMemoryPolicy.h"
#include <cassert>
#include <iostream>
struct WiFi {
  int& heap;
  bool enabled=true;
  bool failOff=false;
  void setEnabled(bool on) {
    if(!on&&failOff)return;
    if(on!=enabled)heap+=on?-50000:50000;
    enabled=on;
  }
};
struct BLE {
  int& heap;
  bool active=false,enabled=false,scanPending=false,failInit=false;
  bool radioActive()const { return active; }
  bool initializationMemoryLow()const { return heap<75000; }
  bool setEnabled(bool on) {
    if(on&&!active&&(initializationMemoryLow()||failInit))return false;
    enabled=on;
    if(on&&!active) { heap-=60000;active=true; }
    if(!on&&active&&!scanPending) { heap+=60000;active=false; }
    return true;
  }
  void finishScanAndShutdown() { heap+=60000;active=false;scanPending=false; }
};
int main() {
  {
    int heap=29000;WiFi wifi{heap};BLE ble{heap};bool paused=false;
    assert(RadioMemoryPolicy::setBLE(ble,wifi,true,true,true,paused));
    assert(ble.enabled&&paused&&!wifi.enabled);
    RadioMemoryPolicy::restore(ble,wifi,true,paused);
    assert(paused&&!wifi.enabled); // Keep WiFi off while BLE owns memory.
    assert(RadioMemoryPolicy::setBLE(ble,wifi,true,true,true,paused)); // Repeat enable.
    assert(RadioMemoryPolicy::setBLE(ble,wifi,false,true,true,paused));
    assert(!paused&&wifi.enabled&&heap==29000);
  }
  {
    int heap=95000;WiFi wifi{heap};BLE ble{heap};bool paused=false;
    assert(RadioMemoryPolicy::setBLE(ble,wifi,true,true,true,paused));
    assert(wifi.enabled&&!paused); // Coexist if there is sufficient RAM.
  }
  {
    int heap=29000;WiFi wifi{heap};BLE ble{heap};bool paused=false;
    assert(!RadioMemoryPolicy::setBLE(ble,wifi,true,false,true,paused));
    assert(wifi.enabled&&!paused&&heap==29000); // Busy WiFi/AP cannot be paused.
    wifi.failOff=true;
    assert(!RadioMemoryPolicy::setBLE(ble,wifi,true,true,true,paused));
    assert(wifi.enabled&&!paused); // Driver refused shutdown.
  }
  {
    int heap=29000;WiFi wifi{heap};BLE ble{heap};bool paused=false;
    ble.failInit=true;
    assert(!RadioMemoryPolicy::setBLE(ble,wifi,true,true,true,paused));
    assert(wifi.enabled&&!paused&&heap==29000); // Rollback failed BLE init.
  }
  {
    int heap=29000;WiFi wifi{heap};BLE ble{heap};bool paused=false;
    assert(RadioMemoryPolicy::setBLE(ble,wifi,true,true,true,paused));
    ble.scanPending=true;
    assert(RadioMemoryPolicy::setBLE(ble,wifi,false,true,true,paused));
    assert(paused&&!wifi.enabled); // Scan cancellation hasn't freed controller yet.
    ble.finishScanAndShutdown();
    RadioMemoryPolicy::restore(ble,wifi,true,paused);
    assert(wifi.enabled&&!paused&&heap==29000);
  }
  {
    int heap=29000;WiFi wifi{heap};BLE ble{heap};bool paused=false;
    assert(RadioMemoryPolicy::setBLE(ble,wifi,true,true,true,paused));
    assert(RadioMemoryPolicy::setBLE(ble,wifi,false,true,false,paused));
    assert(!wifi.enabled&&!paused); // User changed saved WiFi preference to OFF.
  }
  std::cout<<"Radio memory tests passed: low RAM fallback, coexistence, busy WiFi, rollback, delayed shutdown, saved preference\n";
}
