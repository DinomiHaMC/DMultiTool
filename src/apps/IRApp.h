#pragma once
#include "MenuApp.h"
class IRApp:public MenuApp  {
  IRSignal signal;
  uint16_t rgbAddress=0xEF00;
  uint8_t rawFrequency=38;
  IRSignal assigned[12];
  bool assignedSet[12]=  {
  };
  String assignFiles[12];
  int assignIndex=-1;
  void home()override;
  void sender(bool push=true);
  void presets(const String& directory);
  void load(const String& file);
  void remote();
  void rgb();
  void raw();
  void rawText(const String& text);
  void save();
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "Infrared";
  }
  Icon icon()const override  {
    return Icon::IR;
  }
};
