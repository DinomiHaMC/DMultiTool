#pragma once
#include "MenuApp.h"
class UtilsApp:public MenuApp {
  bool audioActive=false,ownsBLE=false,phoneConnected=false;
  String lastCommand;
  void home()override;
  void startAudio();
  void stopAudio();
  void audioPage();
public:
  using MenuApp::MenuApp;
  const char* name()const override { return "Utils"; }
  Icon icon()const override { return Icon::Tool; }
  bool ownsNavigation()const override { return audioActive; }
  void handleInput(InputEvent event)override;
  void update()override;
  void onClose()override { stopAudio(); }
};
