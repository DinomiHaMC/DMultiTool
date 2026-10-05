#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
namespace ClassicNDEF {
struct Card {
  virtual ~Card()=default;
  virtual bool authenticate(uint8_t sector,const uint8_t key[6])=0;
  virtual bool read(uint8_t block,uint8_t data[16])=0;
  virtual bool write(uint8_t block,const uint8_t data[16])=0;
  virtual bool cancelled()const=0;
  virtual void yield()=0;
};
struct Result { bool ok=false;std::string text; };
uint8_t madCRC(const uint8_t* data,size_t length);
Result operate(Card& card,bool write,const std::string& value,bool uri);
}
