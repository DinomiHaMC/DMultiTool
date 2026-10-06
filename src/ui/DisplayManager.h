#pragma once
#include <Adafruit_ST7789.h>
#include "Theme.h"
#include "GameCanvas.h"
#include "Model.h"
#include "KeyboardModel.h"
#include "../games/GameModels.h"
#include "../python/PythonView.h"
#include "../services/Calculator.h"
class DisplayManager  {
  Adafruit_ST7789 tft;
  String previous[32],header,footer,focus,toastText;
  bool selectedBefore[32]=  {
  };
  int oldTop=-1,oldColumns=1,oldCount=-1,oldToastOffset=-1;
  uint8_t themeIndex=255;
  bool invalid=true,asleep=false,barBefore=true;
  bool gridBefore=false;
  bool keyboardVisible=false;
  KeyboardModel previousKeyboard;
  uint8_t keyboardTheme=255;
  bool gameVisible=false,gamePaused=false;
  uint8_t gameTheme=255;
  Games::Board previousGame;
  GameCanvas gameCanvas;
  bool pythonVisible=false;
  uint8_t pythonTheme=255;
  PythonView previousPython;
  bool mediaVisible=false;
  String mediaTitle,mediaHint;
  int mediaX=-1,mediaY=-1,mediaW=0,mediaH=0;
  bool calculatorVisible=false;
  CalculatorModel previousCalculator;
  uint8_t saverMode=255;
  uint32_t saverAt=0,saverSeed=1234567;
  int16_t starX[40]{},starY[40]{};
  int pipeX=0,pipeY=0,pipeDirection=0;
  void keyboardControl(int control,int x,int y,uint16_t color,bool active);
  void icon(Icon icon,int x,int y,uint16_t color);
  void print(const String& text,int x,int y,uint16_t color,uint8_t size=1);
  public:DisplayManager();
  void begin(uint8_t rotation);
  void rotation(uint8_t value);
  void splash();
  void bootStatus(const String& status);
  void sleep(bool on);
  void invalidate()  {
    invalid=true;
  }
  void render(const MenuPage& page,const Status& status,const String& toast,ToastType toastType,uint8_t theme,bool statusBar,bool debug=false,int toastOffset=0);
  void renderKeyboard(const KeyboardModel& keyboard,uint8_t theme);
  void renderGame(const Games::Board& board,uint8_t theme,bool paused=false);
  void renderPython(const PythonView& view,uint8_t theme);
  void renderCalculator(const CalculatorModel& model,uint8_t theme);
  void screensaver(uint8_t mode,uint32_t now,uint8_t theme);
  void notification(const String& text,uint8_t theme);
  bool mediaNeedsRedraw()const { return invalid||!mediaVisible; }
  void beginMedia(int x,int y,int width,int height);
  void mediaBlock(int x,int y,uint16_t* pixels,int width,int height);
  void mediaStatus(const String& title,const String& hint);
  int width()const { return tft.width(); }
  int height()const { return tft.height(); }
};
