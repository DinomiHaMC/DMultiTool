#pragma once
// Radio policy is run by the main task. Preserve the saved WiFi preference;
// a temporary pause only changes the live radio state.
namespace RadioMemoryPolicy {
template<class BLE,class WiFi>
void restore(BLE& ble,WiFi& wifi,bool wanted,bool& paused) {
  if(paused&&!ble.radioActive()) {
    if(wanted)wifi.setEnabled(true);
    paused=false;
  }
}
template<class BLE,class WiFi>
bool setBLE(BLE& ble,WiFi& wifi,bool on,bool mayPause,bool wifiWanted,bool& paused) {
  if(on&&!ble.radioActive()&&ble.initializationMemoryLow()&&wifi.enabled&&mayPause) {
    wifi.setEnabled(false);
    paused=!wifi.enabled;
  }
  bool ok=ble.setEnabled(on);
  if(!on||!ok)restore(ble,wifi,wifiWanted,paused);
  return ok;
}
}
