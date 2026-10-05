#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>
enum class Icon:uint8_t  {
  App,WiFi,BLE,NFC,IR,Folder,Script,Tool,Settings,Info,File,Lock,Game,Python
};
using Action=std::function<void()>;
struct MenuItem  {
  MenuItem(String name="",String info="",Icon image=Icon::App,Action run=  {
  },Action more=  {
  },bool active=true,uint16_t color=0):label(std::move(name)),detail(std::move(info)),icon(image),action(std::move(run)),context(std::move(more)),enabled(active),swatch(color)  {
  }
  String label,detail;
  Icon icon=Icon::App;
  Action action,context;
  bool enabled=true;
  uint16_t swatch=0;
};
struct MenuPage  {
  String title,hint="LEFT Back   OK Select";
  Icon icon=Icon::App;
  std::vector<MenuItem> items;
  int selected=0;
  bool launcher=false;
  uint8_t columns=1;
  Action onBack;
};
struct Status  {
  bool sd=false,wifi=false,connected=false,ble=false,nfc=false;
  uint32_t heap=0,uptime=0;
  uint16_t fps=0;
  bool bleConnected=false;
};
enum class ToastType:uint8_t  {
  Info,Success,Warning,Error
};
