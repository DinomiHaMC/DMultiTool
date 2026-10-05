#include "Buzzer.h"
#include "../pins.h"
void Buzzer::begin()  {
  // ESP32 channels 0/1 share timer 0; leave them to IRremote's allocator.
  // Channel 6 uses timer 3, so UI clicks cannot change the IR carrier.
  setToneChannel(6);
  pinMode(Pins::BUZZER,OUTPUT);
  digitalWrite(Pins::BUZZER,LOW);
}
void Buzzer::beep(bool enabled,uint16_t f,uint16_t ms)  {
  if(!enabled||!ms)return;
  tone(Pins::BUZZER,f);
  started=millis();
  duration=ms;
}
void Buzzer::update()  {
  if(duration && millis()-started>=duration)  {
    noTone(Pins::BUZZER);
    duration=0;
  }
}
