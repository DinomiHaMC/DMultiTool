#pragma once
#include "MenuApp.h"
#include "../services/ScriptParser.h"
class ScriptApp:public MenuApp  {
  File file;
  String output,filename;
  bool running=false,waitingWiFi=false,waitingNFC=false;
  uint32_t started=0,deadline=0,waitStarted=0;
  uint16_t line=0;
  void home()override;
  void start(const String& path);
  void stop(const String& text);
  void show();
  void execute(const String& source);
  public:using MenuApp::MenuApp;
  Action rootBack;
  const char* name()const override  {
    return "Scripts";
  }
  Icon icon()const override  {
    return Icon::Script;
  }
  void update()override;
  void onClose()override;
};
