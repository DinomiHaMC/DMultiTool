#pragma once
#include <stdint.h>
struct Theme  {
  const char* name;
  uint16_t background,panel,foreground,muted,accent,selection,error;
};
class ThemeManager  {
  public:
  static constexpr uint8_t Count=9,Custom=8;
  static const Theme& get(uint8_t index);
  static void setCustom(const uint16_t* colors);
};
