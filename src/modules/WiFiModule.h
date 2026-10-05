#pragma once
#include <WiFi.h>
struct WiFiNetwork  {
  String ssid,bssid;
  int32_t rssi;
  uint8_t channel;
  wifi_auth_mode_t encryption;
};
class WiFiModule  {
  public: bool enabled=false,scanning=false;
  int count=0;
  WiFiNetwork networks[32];
  void begin(bool on);
  void setEnabled(bool on);
  void cancelScan();
  bool scan();
  bool update();
  static const char* auth(wifi_auth_mode_t mode);
};
