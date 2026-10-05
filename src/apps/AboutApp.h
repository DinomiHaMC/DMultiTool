#pragma once
#include "MenuApp.h"
class AboutApp:public MenuApp  {
  void home()override;
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "About";
  }
  Icon icon()const override  {
    return Icon::Info;
  }
};
