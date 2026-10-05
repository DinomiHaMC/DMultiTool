#pragma once
#include "ServiceManager.h"
#include "../ui/UI.h"
class AppManager;
struct Context  {
  ServiceManager& services;
  UI& ui;
  AppManager* apps=nullptr;
};
