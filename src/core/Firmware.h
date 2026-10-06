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
#include "../apps/GamesApp.h"
#include "../apps/PythonApp.h"
#include "../apps/UtilsApp.h"
#include "../apps/ScriptingApp.h"
#include "../apps/MediaApp.h"
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
  GamesApp games { context };
  PythonApp python { context };
  ScriptingApp scripting { context,scripts,python };
  UtilsApp utils { context };
  MediaApp media { context };
  uint32_t statusAt=0;
  uint16_t frames=0;
  bool connected=false;
  uint32_t notificationRevision=0,notificationAt=0;
  bool notificationVisible=false;
  public:void begin();
  void update();
};
