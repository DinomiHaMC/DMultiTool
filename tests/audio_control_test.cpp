#include "../src/services/AudioControl.h"
#include <cassert>
#include <cstdio>
#include <vector>
uint32_t testMillis=0;
int testAdc=4095;
int main() {
  std::vector<uint16_t> sent;
  auto send=[&](uint16_t usage){sent.push_back(usage);return true;};
  using AudioControl::Result;
  for(auto event:{InputEvent::Up,InputEvent::Down,InputEvent::Right,InputEvent::Left,InputEvent::Ok})
    assert(AudioControl::handle(event,true,send)==Result::Sent);
  assert((sent==std::vector<uint16_t>{0xE9,0xEA,0xB5,0xB6,0xCD}));
  sent.clear();
  assert(AudioControl::handle(InputEvent::OkLong,false,send)==Result::Exit);
  assert(AudioControl::handle(InputEvent::OkLong,true,send)==Result::Exit);
  assert(AudioControl::handle(InputEvent::BackLong,true,send)==Result::Ignored);
  assert(AudioControl::handle(InputEvent::None,true,send)==Result::Ignored);
  for(auto event:{InputEvent::Up,InputEvent::Down,InputEvent::Left,InputEvent::Right,InputEvent::Ok})
    assert(AudioControl::handle(event,false,send)==Result::Disconnected);
  assert(sent.empty());
  assert(AudioControl::handle(InputEvent::Ok,true,[](uint16_t){return false;})==Result::Failed);
  // Actual ADC driver: long OK exits once and release cannot send Play/Pause.
  Keyboard keyboard;Settings settings;int exits=0;
  auto hold=[&](int adc,int duration) {
    testAdc=adc;
    for(int elapsed=0;elapsed<duration;elapsed+=5) {
      testMillis+=5;
      if(AudioControl::handle(keyboard.poll(settings),true,send)==Result::Exit)exits++;
    }
  };
  hold(4095,50);hold(2800,settings.longPress+200);hold(4095,50);
  assert(exits==1&&sent.empty());
  hold(2800,100);hold(4095,50);
  assert((sent==std::vector<uint16_t>{0xCD}));
  sent.clear();hold(0,100);hold(4095,50);
  assert((sent==std::vector<uint16_t>{0xB6}));
  sent.clear();hold(0,settings.longPress+200);hold(4095,50);
  assert(exits==1&&sent.empty());
  hold(448,1000);hold(4095,50);
  assert(sent.size()>1);
  for(auto usage:sent)assert(usage==0xE9);
  std::puts("AudioCtrl tests passed: five commands, disconnected/failure, long OK exit without play/pause, LEFT short/long, volume repeat");
}
