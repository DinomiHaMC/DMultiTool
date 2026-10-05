#pragma once
#include <cstddef>
#include <cstdint>
struct BLEFrame { uint16_t size=0;uint8_t bytes[200]{}; };
class BridgeChannel {
public:
  virtual ~BridgeChannel()=default;
  virtual bool connected()const=0;
  virtual bool receive(BLEFrame& frame)=0;
  virtual bool transmit(const uint8_t* bytes,size_t size)=0;
};
