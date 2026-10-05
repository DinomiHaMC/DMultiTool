#pragma once
#include "BridgeChannel.h"
#include "../modules/SDModule.h"
class FileBridge {
  File file;
  String target,temp,outgoing;
  uint32_t replyOffset=0,crc=0xFFFFFFFF,expected=0,received=0,at=0,lastSend=0;
  bool sending=false,receiving=false,waitingACK=false;
  void abort();
  void reply(const String& text);
  void frame(const BLEFrame& frame,SDModule& sd,bool notifications);
public:
  bool active=false;
  String status="Waiting for PC",notification;
  uint32_t notificationRevision=0;
  void enable(bool on) { abort();active=on;status=on?"Waiting for PC":"Stopped"; }
  void update(BridgeChannel& ble,SDModule& sd,bool notifications);
};
