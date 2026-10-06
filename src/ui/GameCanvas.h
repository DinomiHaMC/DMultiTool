#pragma once
#include <Adafruit_GFX.h>
// A small static RGB565 patch: no framebuffer allocation from the shared heap.
class GameCanvas:public GFXcanvas16 {
  uint16_t pixels[16*16]{};
public:
  GameCanvas():GFXcanvas16(16,16,false) { buffer=pixels;setTextWrap(false); }
};
