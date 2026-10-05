#include "AppManager.h"
void AppManager::add(App& app)  {
  if(count<16)registry[count++]=&app;
}
bool AppManager::open(const String& name)  {
  for(int i=0;i<count;i++)if(name==registry[i]->name())  {
    if(current)current->onClose();
    context.ui.reset();
    current=registry[i];
    current->onOpen();
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
  if(e==InputEvent::BackLong&&(!current||!current->ownsNavigation()))  {
    launcher();
    return;
  }
  if(current)current->handleInput(e);
}
App* AppManager::find(const String& name)  {
  for(int i=0;i<count;i++)if(name==registry[i]->name())return registry[i];
  return nullptr;
}
