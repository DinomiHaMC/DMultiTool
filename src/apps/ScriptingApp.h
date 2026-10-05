#pragma once
#include "ScriptApp.h"
#include "PythonApp.h"
class ScriptingApp:public MenuApp {
  ScriptApp& scripts;
  PythonApp& python;
  App* child=nullptr;
  void home()override {
    child=nullptr;
    auto p=menu("Scripts");
    item(p,"Scripts",[this]{child=&scripts;scripts.onOpen();},"/scripts/*.script");
    item(p,"Python",[this]{child=&python;python.onOpen();},"/python | PikaPython + tkinter");
    ui.page(std::move(p),false);
  }
public:
  ScriptingApp(Context& c,ScriptApp& scriptApp,PythonApp& pythonApp):MenuApp(c),scripts(scriptApp),python(pythonApp) {
    scripts.rootBack=[this]{scripts.onClose();home();};
    python.rootBack=[this]{python.onClose();home();};
  }
  const char* name()const override { return "Scripts"; }
  void onOpenAlias(const String& name)override { if(name=="Python") { child=&python;python.onOpen(); }else home(); }
  Icon icon()const override { return Icon::Script; }
  bool ownsNavigation()const override { return child&&child->ownsNavigation(); }
  void handleInput(InputEvent e)override { if(child)child->handleInput(e);else ui.handle(e); }
  void draw()override { if(child)child->draw();else ui.draw(); }
  void update()override { if(child)child->update(); }
  void onClose()override { if(child)child->onClose();child=nullptr; }
};
