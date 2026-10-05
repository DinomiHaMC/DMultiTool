#pragma once
#include <Arduino.h>
#include "../storage/ConfigStore.h"
enum class InputEvent  {
  None,Up,Down,Left,Right,Ok,OkLong,BackLong
};
constexpr InputEvent INPUT_NONE=InputEvent::None,INPUT_UP=InputEvent::Up,INPUT_DOWN=InputEvent::Down,INPUT_LEFT=InputEvent::Left,INPUT_RIGHT=InputEvent::Right,INPUT_OK=InputEvent::Ok,INPUT_OK_LONG=InputEvent::OkLong;
enum class Key  {
  None, Up, Down, Left, Right, Ok, Unknown
};
class Keyboard  {
  Key candidate=Key::None, stable=Key::Unknown;
  uint32_t sampled=0, changed=0, pressed=0, repeated=0;
  bool longSent=false, armed=false, engaged=false;
  public: int raw=4095;
  bool seen=false;
  static Key decode(int value);
  void begin();
  InputEvent poll(const Settings& settings);
  Key held()const { return engaged?stable:Key::None; }
};
