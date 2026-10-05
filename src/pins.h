#pragma once
#include <stdint.h>
namespace Pins  {
  constexpr uint8_t SCK=18, MISO=19, MOSI=23;
  constexpr uint8_t TFT_CS=5, TFT_DC=27, TFT_RST=26, SD_CS=13;
  constexpr uint8_t SDA=21, SCL=22, KEYBOARD=36, BUZZER=16, IR_TX=17;
  constexpr uint8_t PN532_ADDRESS=0x24;
}
