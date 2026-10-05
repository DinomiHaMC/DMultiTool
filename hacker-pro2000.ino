#include "src/core/Firmware.h"
Firmware firmware;
void setup()  {
  firmware.begin();
}
void loop()  {
  firmware.update();
}
