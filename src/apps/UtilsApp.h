#pragma once
#include "MenuApp.h"
#include "../services/Calculator.h"
class UtilsApp:public MenuApp {
  bool audioActive=false,ownsBLE=false,phoneConnected=false;
  String lastCommand;
  CalculatorModel calculator;
  bool calculatorActive=false;
  bool transferActive=false,transferOwnsBLE=false;
  uint32_t transferRefresh=0;
  void transfer();
  void transferPage();
  void notifications(bool push=true);
  void home()override;
  void startAudio();
  void stopAudio();
  void audioPage();
public:
  using MenuApp::MenuApp;
  const char* name()const override { return "Utils"; }
  Icon icon()const override { return Icon::Tool; }
  bool ownsNavigation()const override { return audioActive||calculatorActive||transferActive; }
  void handleInput(InputEvent event)override;
  void update()override;
  void draw()override { if(calculatorActive) { s.display.renderCalculator(calculator,s.config.values.theme);ui.dirty=false; }else ui.draw(); }
  void onClose()override;
};
