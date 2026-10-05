#pragma once
#include "MenuApp.h"
#include "../python/PythonRuntime.h"
class PythonApp:public MenuApp {
  PythonRuntime runtime;
  String directory="/python";
  uint32_t offset=0;
  bool active=false,confirming=false,exiting=false;
  void home()override;
  void browse(const String& path,uint32_t page=0);
  void run(const String& path);
public:
  using MenuApp::MenuApp;
  const char* name()const override { return "Python"; }
  Icon icon()const override { return Icon::Python; }
  bool ownsNavigation()const override { return active; }
  void update()override;
  void draw()override;
  void handleInput(InputEvent event)override;
  void onClose()override { runtime.stop();active=confirming=exiting=false; }
};
