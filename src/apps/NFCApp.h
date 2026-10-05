#pragma once
#include "MenuApp.h"
class NFCApp:public MenuApp  {
  bool waiting=false,ndefWaiting=false;
  uint32_t ndefVersion=0;
  void home()override;
  void scan();
  void info();
  void save();
  void ndefRead();
  void ndefWrite(bool uri);
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "NFC";
  }
  Icon icon()const override  {
    return Icon::NFC;
  }
  void onClose()override;
  void update()override;
};
