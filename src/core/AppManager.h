#pragma once
#include "App.h"
#include "Context.h"
class AppManager  {
  Context& context;
  App* registry[16]=  {
  };
  int count=0;
  App* current=nullptr;
  public:explicit AppManager(Context& c):context(c)  {
    context.apps=this;
  }
  App* find(const String& name);
  void add(App& app);
  bool open(const String& name);
  bool openFile(const String& name,const String& path);
  void launcher();
  void update();
  void draw();
  void handleInput(InputEvent event);
  bool foregroundBusy()const { return current&&current->ownsNavigation(); }
  int size()const  {
    return count;
  }
  App* at(int index)  {
    return index>=0&&index<count?registry[index]:nullptr;
  }
};
