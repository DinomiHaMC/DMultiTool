#pragma once
#include "MenuApp.h"
class SettingsApp:public MenuApp  {
  void home()override;
  void display(bool push=true);
  void sound(bool push=true);
  void input(bool push=true);
  void boot(bool push=true);
  void developer(bool push=true);
  void save();
  void themes();
  void themeEditor(bool push=true);
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "Settings";
  }
  Icon icon()const override  {
    return Icon::Settings;
  }
};
