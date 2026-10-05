#include "WiFiModule.h"
#include <esp_wifi.h>
#include "../config.h"
void WiFiModule::begin(bool on)  {
  WiFi.persistent(false);
  setEnabled(on);
  LOG_INFO("BOOT","WiFi ready");
}
void WiFiModule::setEnabled(bool on)  {
  if(scanning)  {
    WiFi.scanDelete();
    scanning=false;
  }
  count=0;
  bool ok=WiFi.mode(on?WIFI_STA:WIFI_OFF);
  enabled=ok&&on;
  if(!ok)LOG_ERROR("WiFi","Mode change failed");
}
bool WiFiModule::scan()  {
  if(!enabled||scanning)return false;
  WiFi.scanDelete();
  int r=WiFi.scanNetworks(true,true);
  scanning=r==WIFI_SCAN_RUNNING;
  if(!scanning)LOG_ERROR("WiFi","Scan failed %d",r);
  return scanning;
}
bool WiFiModule::update()  {
  if(!scanning)return false;
  int n=WiFi.scanComplete();
  if(n==WIFI_SCAN_RUNNING)return false;
  scanning=false;
  count=n<0?0:min(n,32);
  for(int i=0;i<count;i++)networks[i]=  {
    WiFi.SSID(i),WiFi.BSSIDstr(i),WiFi.RSSI(i),(uint8_t)WiFi.channel(i),WiFi.encryptionType(i)
  };
  WiFi.scanDelete();
  LOG_INFO("WiFi","Scan complete: %d",n);
  return true;
}
const char* WiFiModule::auth(wifi_auth_mode_t m)  {
  switch(m)  {
    case WIFI_AUTH_OPEN:return "Open";
    case WIFI_AUTH_WEP:return "WEP";
    case WIFI_AUTH_WPA_PSK:return "WPA";
    case WIFI_AUTH_WPA2_PSK:return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE:return "WPA2 EAP";
    case WIFI_AUTH_WPA3_PSK:return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:return "WPA2/WPA3";
    default:return "Other";
  }
}
void WiFiModule::cancelScan()  {
  if(scanning)esp_wifi_scan_stop();
}
