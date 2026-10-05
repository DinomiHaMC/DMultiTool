#pragma once
#include <Adafruit_ST7789.h>
#include "Theme.h"
#include "Model.h"
#include "KeyboardModel.h"
#include "../games/GameModels.h"
#include "../python/PythonView.h"
class DisplayManager  {
  Adafruit_ST7789 tft;
  String previous[32],header,footer,focus,toastText;
  bool selectedBefore[32]=  {
  };
  int oldTop=-1,oldColumns=1,oldCount=-1,oldToastOffset=-1;
  uint8_t themeIndex=255;
  bool invalid=true,asleep=false,barBefore=true;
  bool keyboardVisible=false;
  KeyboardModel previousKeyboard;
  uint8_t keyboardTheme=255;
  bool gameVisible=false,gamePaused=false;
  uint8_t gameTheme=255;
  Games::Board previousGame;
  bool pythonVisible=false;
  uint8_t pythonTheme=255;
  PythonView previousPython;
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
  int width()const { return tft.width(); }
  int height()const { return tft.height(); }
};
