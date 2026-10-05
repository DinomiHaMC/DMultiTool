#pragma once
#include "MenuApp.h"
class FileManagerApp:public MenuApp  {
  String path="/",opened;
  uint32_t offset=0,pageOffset=0,nextOffset=0;
  std::vector<uint32_t> positions;
  void home()override;
  void browse(const String& p,uint32_t start=0);
  void openFile(const String& p);
  void view();
  void context(const String& p,bool directory);
  void mkdir();
  public:using MenuApp::MenuApp;
  const char* name()const override  {
    return "Files";
  }
  Icon icon()const override  {
    return Icon::Folder;
  }
  void openDirectory(const String& directory)  {
    browse(directory);
  }
};
