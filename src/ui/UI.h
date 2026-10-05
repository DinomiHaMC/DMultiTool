#pragma once
#include "DisplayManager.h"
#include "../input/Keyboard.h"
#include "KeyboardModel.h"
class UI  {
  DisplayManager& display;
  Settings& settings;
  std::vector<MenuPage> history;
  MenuPage current;
  String toastText;
  ToastType toastType=ToastType::Info;
  uint32_t toastStart=0,animationAt=0;
  std::function<void(String)> inputDone;
  bool editing=false;
  KeyboardModel keyboard;
  public:Status status;
  Action exit;
  bool dirty=true;
  UI(DisplayManager& d,Settings& s):display(d),settings(s)  {
  }
  void reset();
  void page(MenuPage page,bool push=true);
  MenuPage& model()  {
    return current;
  }
  void back();
  bool isInput()const  {
    return editing;
  }
  const KeyboardModel& inputModel()const { return keyboard; }
  void handle(InputEvent event);
  void draw();
  void update();
  void toast(const String& text,ToastType type=ToastType::Info);
  void message(const String& title,const String& text);
  void confirm(const String& title,const String& text,Action yes);
  void progress(const String& title,const String& detail,Action cancel);
  void textInput(const String& title,const String& initial,std::function<void(String)> done,int max=128,bool password=false,int mode=0);
  void numberInput(const String& title,uint32_t initial,uint32_t low,uint32_t high,std::function<void(uint32_t)> done,bool hex=false);
  void rows(const String& title,const String& text,bool push=true);
};
