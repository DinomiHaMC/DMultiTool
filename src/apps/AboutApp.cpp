#include "AboutApp.h"
#include "../core/Version.h"
void AboutApp::home()  {
  ui.rows("About","Hacker Pro 2000\nFirmware: " FW_VERSION "\nCommit: " FW_COMMIT "\nESP32-WROOM-32\nST7789 240x320\nOwn modular Arduino firmware\nUI/UX inspired by portable\nmultitool firmware such as Bruce.\nCredits: Espressif, Adafruit,\nArduino-IRremote.\nBattery: unavailable",false);
}
