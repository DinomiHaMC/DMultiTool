#include "AppManager.h"
void AppManager::add(App& app)  {
  if(count<16)registry[count++]=&app;
}
bool AppManager::open(const String& name)  {
  for(int i=0;i<count;i++)if(name==registry[i]->name()||(name=="Python"&&String(registry[i]->name())=="Scripts"))  {
    if(current)current->onClose();
    context.ui.reset();
    current=registry[i];
    current->onOpenAlias(name);
    return true;
  }
  return false;
}
void AppManager::launcher()  {
  open("Launcher");
}
void AppManager::update()  {
  if(current)current->update();
}
void AppManager::draw()  {
  if(current)current->draw();
}
void AppManager::handleInput(InputEvent e)  {
  if(current)current->handleInput(e);
}
App* AppManager::find(const String& name)  {
  for(int i=0;i<count;i++)if(name==registry[i]->name())return registry[i];
  return nullptr;
}

bool AppManager::openFile(const String& name,const String& path) {
  App* app=find(name);if(!app)return false;
  if(current)current->onClose();
  context.ui.reset();current=app;current->onOpenFile(path);return true;
}
