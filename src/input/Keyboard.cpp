#include "Keyboard.h"
#include "../pins.h"
#include "../config.h"
Key Keyboard::decode(int v)  {
  if(v>=0 && v<=200) return Key::Left;
  if(v>=300 && v<=700) return Key::Up;
  if(v>=900 && v<=1500) return Key::Down;
  if(v>=1600 && v<=2300) return Key::Right;
  if(v>=2500 && v<=3200) return Key::Ok;
  return v>3900 ? Key::None : Key::Unknown;
}
void Keyboard::begin()  {
  pinMode(Pins::KEYBOARD,INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(Pins::KEYBOARD,ADC_11db);
}
InputEvent Keyboard::poll(const Settings& s)  {
  const uint32_t now=millis();
  if(now-sampled<5) return INPUT_NONE;
  sampled=now;
  raw=analogRead(Pins::KEYBOARD);
  Key key=decode(raw);
  if(key!=candidate)  {
    candidate=key;
    changed=now;
  }
  if(now-changed<Config::DebounceMs) return INPUT_NONE;
  if(stable!=candidate)  {
    Key old=stable;
    stable=candidate;
    if(stable==Key::None)  {
      bool shortOk=engaged && old==Key::Ok && !longSent;
      bool shortBack=engaged && old==Key::Left && !longSent;
      armed=true;
      engaged=false;
      return shortOk?INPUT_OK:shortBack?INPUT_LEFT:INPUT_NONE;
    }
    if(stable==Key::Unknown)  {
      armed=false;
      engaged=false;
      return INPUT_NONE;
    }
    if(!armed)  {
      engaged=false;
      return INPUT_NONE;
    }
    armed=false;
    engaged=true;
    seen=true;
    pressed=repeated=now;
    longSent=false;
    switch(stable)  {
      case Key::Up:return INPUT_UP;
      case Key::Down:return INPUT_DOWN;
      case Key::Left:break;
      case Key::Right:return INPUT_RIGHT;
      default:break;
    }
  }
  if(engaged && (stable==Key::Ok||stable==Key::Left) && !longSent && now-pressed>=s.longPress)  {
    longSent=true;
    return stable==Key::Ok?InputEvent::OkLong:InputEvent::BackLong;
  }
  if(engaged && s.repeat && (stable==Key::Up || stable==Key::Down) && now-pressed>=s.repeatDelay && now-repeated>=s.repeatRate)  {
    repeated=now;
    return stable==Key::Up?INPUT_UP:INPUT_DOWN;
  }
  return INPUT_NONE;
}
