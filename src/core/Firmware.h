#pragma once
#include "ServiceManager.h"
#include "AppManager.h"
#include "IdleManager.h"
#include "../apps/LauncherApp.h"
#include "../apps/WiFiApp.h"
#include "../apps/BLEApp.h"
#include "../apps/NFCApp.h"
#include "../apps/IRApp.h"
#include "../apps/FileManagerApp.h"
#include "../apps/ScriptApp.h"
#include "../apps/ToolsApp.h"
#include "../apps/SettingsApp.h"
#include "../apps/AboutApp.h"
#include "../apps/GamesApp.h"
#include "../apps/PythonApp.h"
#include "../apps/UtilsApp.h"
class Firmware  {
  ServiceManager services;
  UI ui  {
    services.display,services.config.values
  };
  Context context  {
    services,ui
  };
  AppManager apps  {
    context
  };
  IdleManager idle;
  LauncherApp launcher  {
    context
  };
  WiFiApp wifi  {
    context
  };
  BLEApp ble  {
    context
  };
  NFCApp nfc  {
    context
  };
  IRApp ir  {
    context
  };
  FileManagerApp files  {
    context
  };
  ScriptApp scripts  {
    context
  };
  ToolsApp tools  {
    context
  };
  SettingsApp settings  {
    context
  };
  AboutApp about  {
    context
  };
  GamesApp games { context };
  PythonApp python { context };
  UtilsApp utils { context };
  uint32_t statusAt=0;
  uint16_t frames=0;
  bool connected=false;
  public:void begin();
  void update();
};
