#pragma once
#include <Arduino.h>
namespace Config  {
  constexpr uint32_t DebounceMs=35, RepeatDelayMs=450, RepeatMs=140;
  constexpr size_t MaxEntries=64, MaxRows=66;
  constexpr uint32_t SdFrequency=10000000;
}
void systemLog(uint8_t level,const char* scope,const char* format,...);
#define LOG_DEBUG(scope, ...) systemLog(0,scope,__VA_ARGS__)
#define LOG_INFO(scope, ...) systemLog(1,scope,__VA_ARGS__)
#define LOG_WARN(scope, ...) systemLog(2,scope,__VA_ARGS__)
#define LOG_ERROR(scope, ...) systemLog(3,scope,__VA_ARGS__)
