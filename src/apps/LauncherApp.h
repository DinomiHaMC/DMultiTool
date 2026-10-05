#pragma once
#include "MenuApp.h"
class LauncherApp:public MenuApp  {
  void home()override;
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "Launcher";
  }
  Icon icon()const override  {
    return Icon::App;
  }
};
