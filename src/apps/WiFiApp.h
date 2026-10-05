#pragma once
#include "MenuApp.h"
class WiFiApp:public MenuApp  {
  bool scanWaiting=false,jobWaiting=false,connectWaiting=false,apActive=false,captureView=false;
  uint32_t scanVersion=0,jobVersion=0,connectedAt=0,refreshAt=0;
  String connectSsid;
  void home()override;
  void scan();
  void networks(bool connect=false);
  void networkInfo(int index);
  void connect(int index);
  void current();
  void saved();
  void job(NetOperation operation);
  void startJob(NetJob job);
  void accessPoint();
  void capture(bool save);
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "WiFi";
  }
  Icon icon()const override  {
    return Icon::WiFi;
  }
  void update()override;
  void onClose()override;
};
