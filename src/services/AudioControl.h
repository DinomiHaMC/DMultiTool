#pragma once
#include "../input/Keyboard.h"
namespace AudioControl {
enum class Result { Ignored,Exit,Disconnected,Sent,Failed };
// HID Consumer page (0x0C), 16-bit input report 3 of our existing HID device.
inline uint16_t usage(InputEvent event) {
  switch(event) {
    case InputEvent::Up:return 0xE9;
    case InputEvent::Down:return 0xEA;
    case InputEvent::Right:return 0xB5;
    case InputEvent::Left:return 0xB6;
    case InputEvent::Ok:return 0xCD;
    default:return 0;
  }
}
template<class Send>
Result handle(InputEvent event,bool connected,Send send) {
  if(event==InputEvent::OkLong)return Result::Exit;
  const uint16_t command=usage(event);
  if(!command)return Result::Ignored;
  if(!connected)return Result::Disconnected;
  return send(command)?Result::Sent:Result::Failed;
}
}
