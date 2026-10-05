#include <cassert>
#include <cstdio>
#include <vector>
#include "../src/input/Keyboard.h"
uint32_t testMillis=0;
int testAdc=4095;
Settings settings;
std::vector<InputEvent> hold(Keyboard& keyboard,int adc,uint32_t duration){
 std::vector<InputEvent> events;testAdc=adc;
 for(uint32_t t=0;t<duration;t+=5){testMillis+=5;auto e=keyboard.poll(settings);if(e!=INPUT_NONE)events.push_back(e);}
 return events;
}
int main(){
 for(int v=0;v<=4095;v++){
  Key expected=Key::Unknown;
  if(v<=200)expected=Key::Left;else if(v>=300&&v<=700)expected=Key::Up;
  else if(v>=900&&v<=1500)expected=Key::Down;else if(v>=1600&&v<=2300)expected=Key::Right;
  else if(v>=2500&&v<=3200)expected=Key::Ok;else if(v>3900)expected=Key::None;
  assert(Keyboard::decode(v)==expected);
 }
 Keyboard k;k.begin();assert(hold(k,4095,50).empty());
 assert(hold(k,448,20).empty());assert(hold(k,4095,50).empty()); // debounce rejects bounce
 auto e=hold(k,448,100);assert(e.size()==1&&e[0]==INPUT_UP);
 e=hold(k,448,700);assert(e.size()>=2);for(auto v:e)assert(v==INPUT_UP);
 assert(hold(k,4095,50).empty());
 assert(hold(k,2801,200).empty());e=hold(k,4095,50);assert(e.size()==1&&e[0]==INPUT_OK);
 e=hold(k,2801,1100);assert(e.size()==1&&e[0]==INPUT_OK_LONG);
 assert(hold(k,2801,1100).empty());assert(hold(k,4095,50).empty()); // no short OK after long
 hold(k,800,50);assert(hold(k,2801,1100).empty());assert(hold(k,4095,50).empty()); // gaps do not activate keys
 hold(k,448,100);assert(hold(k,2801,1100).empty());assert(hold(k,4095,50).empty()); // direct key transition requires release
 settings.repeat=false;e=hold(k,1167,1500);assert(e.size()==1&&e[0]==INPUT_DOWN);
 hold(k,4095,50);settings.repeat=true;
 e=hold(k,0,1100);assert(e.size()==1&&e[0]==InputEvent::BackLong);assert(hold(k,4095,50).empty());
 assert(hold(k,0,100).empty());e=hold(k,4095,50);assert(e.size()==1&&e[0]==InputEvent::Left);
 testMillis=UINT32_MAX-100;Keyboard wrap;hold(wrap,4095,50);e=hold(wrap,2801,1100);assert(e.size()==1&&e[0]==INPUT_OK_LONG);assert(hold(wrap,4095,50).empty());
 std::puts("Keyboard tests passed: ranges, debounce, short/long, repeat, gaps, rollover");
}
