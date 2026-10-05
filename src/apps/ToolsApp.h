#pragma once
#include "MenuApp.h"
class ToolsApp:public MenuApp  {
  Action refresh;
  uint32_t refreshed=0;
  void home()override;
  void system();
  void hardware();
  void i2c();
  void adc();
  void memory();
  void gpio();
  void spi();
  void benchmark();
  void serial();
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "Tools";
  }
  Icon icon()const override  {
    return Icon::Tool;
  }
  void update()override;
  void onClose()override  {
    refresh=nullptr;
  }
};
