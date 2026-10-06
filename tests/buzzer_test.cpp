#include "../src/modules/Buzzer.h"
#include <cassert>
#include <iostream>
int main() {
  Buzzer buzzer;buzzer.begin();assert(toneChannel==6);
  buzzer.beep(true,2200,35);assert(toneFrequency==2200);
  testToneMillis=36;buzzer.update();assert(toneFrequency==0);
  buzzer.beginKey();buzzer.key(true);assert(toneFrequency==700);
  int starts=toneStarts,stops=toneStops;
  buzzer.key(true);buzzer.beep(true,2400,60);testToneMillis+=10000;buzzer.update();
  assert(toneFrequency==700&&starts==toneStarts&&stops==toneStops);
  buzzer.key(false);assert(toneFrequency==0&&toneStops==stops+1);
  buzzer.key(true);assert(toneFrequency==700);buzzer.endKey();assert(toneFrequency==0);
  buzzer.key(true);assert(toneFrequency==0);
  buzzer.beep(true,2200,35);assert(toneFrequency==2200);
  buzzer.beginKey();assert(toneFrequency==0);buzzer.key(true);buzzer.endKey();assert(toneFrequency==0);
  std::cout<<"Buzzer tests passed: timer separation, normal beep duration, continuous Morse hold, repeated press/background beep ignored, release/exit/re-entry cleanup\n";
}
