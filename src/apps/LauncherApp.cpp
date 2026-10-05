#include "LauncherApp.h"
void LauncherApp::home()  {
  auto p=menu("Hacker Pro 2000");
  p.launcher=true;
  for(int i=0;i<ctx.apps->size();i++)  {
    App* app=ctx.apps->at(i);
    if(String(app->name())=="Launcher")continue;
    String name=app->name();
    p.items.push_back(  {
      name,"Open application",app->icon(),[this,name]  {
        ctx.apps->open(name);
      }
    }
    );
  }
  p.items.push_back(  {
    "SubGHz","Not installed",Icon::Lock,  {
    },  {
    },false
  }
  );
  p.items.push_back(  {
    "2.4GHz Radio","Not installed",Icon::Lock,  {
    },  {
    },false
  }
  );
  p.hint="OK Open  LONG LEFT Home";
  ui.page(std::move(p),false);
}
