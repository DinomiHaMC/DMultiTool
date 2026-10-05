#pragma once
#include <array>
#include <cstdint>
struct PythonWidget { char text[96]{};bool button=false; };
struct PythonRectangle { int16_t x1=0,y1=0,x2=0,y2=0;uint16_t color=0; };
struct PythonView {
  char title[64]="Python",status[64]="Starting",console[128]{};
  std::array<PythonWidget,12> widgets{};
  std::array<PythonRectangle,24> rectangles{};
  uint8_t count=0,rectanglesCount=0;
  int selected=-1;
};
