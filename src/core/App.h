#pragma once
#include "../input/Keyboard.h"
#include "../ui/Model.h"
class App  {
  public:virtual ~App()=default;
  virtual const char* name()const=0;
  virtual Icon icon()const=0;
  // Foreground apps can reserve all five buttons and require confirmed exit.
  virtual bool ownsNavigation()const { return false; }
  virtual void onOpen()=0;
  virtual void onOpenAlias(const String&) { onOpen(); }
  virtual void onOpenFile(const String&) { onOpen(); }
  virtual void onClose()  {
  }
  virtual void update()  {
  }
  virtual void draw()=0;
  virtual void handleInput(InputEvent event)=0;
};
