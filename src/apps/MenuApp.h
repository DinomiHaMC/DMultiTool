#pragma once
#include "../core/App.h"
#include "../core/Context.h"
#include "../core/AppManager.h"
class MenuApp:public App  {
  protected:Context& ctx;
  ServiceManager& s;
  UI& ui;
  virtual void home()=0;
  MenuPage menu(const String& title)  {
    MenuPage p;
    p.title=title;
    p.icon=icon();
    return p;
  }
  void item(MenuPage& p,const String& label,Action action,const String& detail="",Action context=  {
  }
  )  {
    p.items.push_back(  {
      label,detail,icon(),action,context
    }
    );
  }
  void report(bool ok,const String& success="Done",const String& error="Operation failed")  {
    ui.toast(ok?success:error,ok?ToastType::Success:ToastType::Error);
  }
  public:explicit MenuApp(Context& c):ctx(c),s(c.services),ui(c.ui)  {
  }
  void onOpen()override  {
    home();
  }
  void draw()override  {
    ui.draw();
  }
  void handleInput(InputEvent e)override  {
    ui.handle(e);
  }
};
