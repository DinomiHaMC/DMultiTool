#pragma once
#include "../display-stubs/Arduino.h"
class Stream {
public:
  virtual ~Stream()=default;
  virtual int available()=0;virtual int read()=0;virtual int peek()=0;
  virtual size_t write(uint8_t byte)=0;virtual size_t write(const uint8_t* bytes,size_t size)=0;
};
