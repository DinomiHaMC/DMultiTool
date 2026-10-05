#include "LauncherApp.h"
void LauncherApp::home()  {
  auto p=menu("DMultiTool");
  p.launcher=true;
  for(int i=0;i<ctx.apps->size();i++)  {
    App* app=ctx.apps->at(i);
    if(String(app->name())=="Launcher")continue;
    String name=app->name();
    p.items.push_back({name,"Open application",app->icon(),[this,name]{ctx.apps->open(name);}});
  }
  p.hint="Arrows Move | OK Open | Hold OK Back";
  ui.page(std::move(p),false);
}
