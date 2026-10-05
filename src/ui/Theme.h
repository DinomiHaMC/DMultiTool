#pragma once
#include <stdint.h>
struct Theme  {
  const char* name;
  uint16_t background,panel,foreground,muted,accent,selection,error;
};
class ThemeManager  {
  public:static const Theme& get(uint8_t index);
};
