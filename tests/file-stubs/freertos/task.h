#pragma once
#include "FreeRTOS.h"
#include <thread>
#include <vector>
inline std::vector<std::thread> fakeTasks;
inline bool failTask=false;
inline int xTaskCreate(void (*fn)(void*),const char*,int,void* context,int,void*) { if(failTask)return 0;fakeTasks.emplace_back(fn,context);return pdPASS; }
inline void vTaskDelete(void*) {}
inline void joinTasks() { for(auto& task:fakeTasks)task.join();fakeTasks.clear(); }
