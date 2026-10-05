#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// Complete each I2C exchange before issuing the next PN532 command.
// Adafruit 1.3.4's RFConfiguration helper waits for the reply, but does not
// consume it. Keep the correction here, without patching installed libraries.
namespace PN532Polling {
constexpr uint16_t ResponseTimeoutMs=500;
constexpr uint8_t PassiveRetries=2;

template<class Reader,class Bus>
bool configure(Reader& reader,Bus& bus,uint8_t address) {
  uint8_t command[]={0x32,0x05,0xFF,0x01,PassiveRetries};
  if(!reader.sendCommandCheckAck(command,sizeof(command),ResponseTimeoutMs))return false;
  // I2C status byte + complete RFConfiguration response, including DCS/postamble.
  uint8_t response[10]{};
  size_t count=bus.requestFrom(address,sizeof(response));
  for(size_t i=0;i<count;i++) {
    int value=bus.read();
    if(i<sizeof(response))response[i]=(uint8_t)value;
  }
  const uint8_t expected[]={1,0,0,0xFF,2,0xFE,0xD5,0x33,0xF8,0};
  return count==sizeof(response)&&memcmp(response,expected,sizeof(expected))==0;
}

template<class Reader>
bool readTag(Reader& reader,uint8_t baud,uint8_t* uid,uint8_t& length) {
  // The installed library copies the chip's UID length without a capacity.
  // Receive into a defensive buffer and publish only supported complete UIDs.
  uint8_t raw[256]{};
  uint8_t found=0;
  length=0;
  if(!reader.readPassiveTargetID(baud,raw,&found,ResponseTimeoutMs))return false;
  if(found!=4&&found!=7)return false;
  memcpy(uid,raw,found);length=found;return true;
}
// Read the complete target frame as well as SAK/ATQA: memory commands must
// match the selected tag technology, not be guessed from the UID alone.
template<class Reader,class Bus>
bool selectTag(Reader& reader,Bus& bus,uint8_t address,uint8_t baud,
               uint8_t* uid,uint8_t& length,uint16_t& atqa,uint8_t& sak) {
  length=0;atqa=0;sak=0;
  uint8_t command[]={0x4A,1,baud};
  if(!reader.sendCommandCheckAck(command,sizeof(command),ResponseTimeoutMs))return false;
  uint8_t response[64]{};
  size_t count=bus.requestFrom(address,sizeof(response));
  for(size_t i=0;i<count;i++) { int v=bus.read();if(i<sizeof(response))response[i]=(uint8_t)v; }
  const uint8_t* frame=response+1;
  if(count<11||response[0]!=1||frame[0]||frame[1]||frame[2]!=0xFF)return false;
  size_t payload=frame[3];
  if(payload<3||payload+8>count||payload+8>sizeof(response)||(uint8_t)(frame[3]+frame[4]))return false;
  uint8_t sum=0;for(size_t i=5;i<=5+payload;i++)sum+=frame[i];
  if(sum||frame[6+payload]||frame[5]!=0xD5||frame[6]!=0x4B||frame[7]!=1||payload<8)return false;
  uint8_t found=frame[12];
  if(frame[8]!=1||(found!=4&&found!=7)||payload<(size_t)(8+found))return false;
  memcpy(uid,frame+13,found);length=found;atqa=(frame[9]<<8)|frame[10];sak=frame[11];
  return true;
}

}
