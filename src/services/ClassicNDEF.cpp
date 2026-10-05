#include "ClassicNDEF.h"
#include "NDEFCodec.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <string>
namespace ClassicNDEF {
static constexpr uint8_t MADKey[]={0xA0,0xA1,0xA2,0xA3,0xA4,0xA5};
static constexpr uint8_t NDEFKey[]={0xD3,0xF7,0xD3,0xF7,0xD3,0xF7};
static constexpr uint8_t FactoryKey[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

static bool authenticate(Card& card,uint8_t sector,const uint8_t key[6]) {
  if(card.authenticate(sector,key))return true;
  // Some phone formatters preserve factory Key A for an otherwise valid
  // MAD1/NDEF card. This is a fixed interoperability fallback, never a
  // key search, and is attempted only for a sector already marked 03 E1.
  return memcmp(key,FactoryKey,sizeof(FactoryKey))&&card.authenticate(sector,FactoryKey);
}

static std::string at(const char* what,uint8_t value) {
  return std::string("Classic: ")+what+" "+std::to_string(value);
}
uint8_t madCRC(const uint8_t* data,size_t length) {
  // MSB-first representation of the MAD CRC register, verified against
  // AN10787's 0x89 sample and the canonical all-NDEF MAD1 (CRC 0x14).
  uint8_t crc=0xC7;
  for(size_t i=0;i<length;i++) {
    crc^=data[i];
    for(int b=0;b<8;b++)crc=(crc<<1)^((crc&0x80)?0x1D:0);
  }
  return crc;
}
static bool access(const uint8_t* trailer,bool writing) {
  uint8_t c1=trailer[7]>>4,c2=trailer[8]&15,c3=trailer[8]>>4;
  if(((trailer[6]&15)^c1)!=15||((trailer[6]>>4)^c2)!=15||((trailer[7]&15)^c3)!=15)return false;
  uint8_t gpb=trailer[9];
  if((gpb&0xF0)!=0x40||(gpb&12)!=0||(writing&&(gpb&3)))return false;
  for(int block=0;block<3;block++) {
    int code=(((c1>>block)&1)<<2)|(((c2>>block)&1)<<1)|((c3>>block)&1);
    if(writing?code!=0:(code==3||code==5||code==7))return false;
  }
  return true;
}
Result operate(Card& card,bool writing,const std::string& value,bool uri) {
  auto error=[](const char* text) { return Result{false,text}; };
  if(card.cancelled())return error("Cancelled");
  if(!authenticate(card,0,MADKey))return error("Classic: MAD key unavailable; format NDEF with phone first");
  uint8_t mad[32]{},trailer[16]{};
  if(!card.read(3,trailer))return error("Classic: MAD trailer read failed");
  if(!card.read(1,mad))return error("Classic: MAD block 1 read failed");
  if(!card.read(2,mad+16))return error("Classic: MAD block 2 read failed");
  if(!(trailer[9]&0x80)||(trailer[9]&3)!=1||mad[0]!=madCRC(mad+1,31))return error("Classic: valid MAD1 directory required");
  std::array<uint8_t,45> blocks{};size_t count=0;bool gap=false,started=false;
  for(int sector=1;sector<16;sector++) {
    bool ndef=mad[sector*2]==3&&mad[sector*2+1]==0xE1;
    if(!ndef) { if(started)gap=true;continue; }
    if(gap)return error("Classic: non-contiguous NDEF sectors unsupported");
    started=true;for(int b=0;b<3;b++)blocks[count++]=sector*4+b;
  }
  if(!count)return error("Classic: no NDEF sectors; format with phone first");
  std::array<uint8_t,720> data{};
  std::string failure;
  auto loadSector=[&](size_t i,bool writable)->bool {
    if(card.cancelled()) { failure="Cancelled";return false; }
    uint8_t sector=blocks[i]/4;
    if(!authenticate(card,sector,NDEFKey)) { failure=at("NDEF key unavailable for sector",sector);return false; }
    if(!card.read(blocks[i]+3,trailer)) { failure=at("trailer read failed at sector",sector);return false; }
    if(!access(trailer,writable)) { failure=writable?at("read-only/incompatible access bits at sector",sector):at("unsupported mapping/read access at sector",sector);return false; }
    for(int b=0;b<3;b++) {
      if(card.cancelled()) { failure="Cancelled";return false; }
      if(!card.read(blocks[i+b],data.data()+(i+b)*16)) { failure=at("data read failed at block",blocks[i+b]);return false; }
      card.yield();
    }
    return true;
  };
  const size_t capacity=count*16;
  if(!writing) {
    // A phone commonly writes a short record into the first allocated NFC
    // sector. Do not reject it merely because a later allocated sector is
    // locked or uses a different key.
    for(size_t loaded=0;loaded<count;loaded+=3) {
      if(!loadSector(loaded,false))return error(failure.c_str());
      std::string text;
      if(NDEF::decode(data.data(),(loaded+3)*16,text))return {true,text};
    }
    return error("Classic: empty/unsupported NDEF message");
  }
  auto encoded=NDEF::encode(value,uri);
  if(encoded.empty())return error("Classic: invalid NDEF value");
  size_t loaded=0;
  auto ensure=[&](size_t bytes)->bool {
    while(loaded*16<bytes&&loaded<count) {
      if(!loadSector(loaded,true))return false;
      loaded+=3;
    }
    return loaded*16>=bytes;
  };
  if(!ensure(16))return error(failure.c_str());
  size_t offset=0;while(offset<loaded*16&&data[offset]==0)offset++;
  if(offset+2>capacity||data[offset]!=3)return error("Classic: unsupported TLV layout; format NDEF with phone");
  size_t oldLength=data[offset+1],header=2;
  if(oldLength==255) {
    if(!ensure(offset+4))return error(failure.c_str());
    oldLength=(data[offset+2]<<8)|data[offset+3];header=4;
  }
  if(oldLength>capacity-offset-header)return error("Classic: invalid NDEF length");
  size_t after=offset+header+oldLength;
  size_t required=std::max(after+1,offset+encoded.size());
  if(required>capacity)return error("Classic: NDEF too large for allocated sectors");
  if(!ensure(required))return error(failure.c_str());
  size_t checked=after;
  while(checked<loaded*16&&data[checked]==0)checked++;
  if(checked<loaded*16&&data[checked]!=0xFE)return error("Classic: extra TLVs present; write refused");
  // Clear the length first; write and verify body blocks; publish length last.
  size_t first=offset/16,last=(offset+encoded.size()-1)/16,publish=(offset+1)/16;
  uint8_t empty[16];memcpy(empty,data.data()+publish*16,16);
  empty[(offset+1)%16]=0;
  if((offset+2)/16==publish)empty[(offset+2)%16]=0xFE;
  int authenticated=-1;
  auto writeVerified=[&](size_t index,const uint8_t* source)->bool {
    if(card.cancelled())return false;
    int sector=blocks[index]/4;
    if(authenticated!=sector) {
      if(!authenticate(card,sector,NDEFKey))return false;
      authenticated=sector;
    }
    if(!card.write(blocks[index],source))return false;
    uint8_t verify[16]{};
    if(!card.read(blocks[index],verify)||memcmp(verify,source,16))return false;
    card.yield();return true;
  };
  if(!writeVerified(publish,empty))return error("Classic: clearing NDEF length failed / cancelled");
  memcpy(data.data()+offset,encoded.data(),encoded.size());
  for(size_t i=first;i<=last;i++) {
    if(i==publish)continue;
    if(!writeVerified(i,data.data()+i*16))return error("Classic: interrupted/failed body write; NDEF empty");
  }
  if(!writeVerified(publish,data.data()+publish*16))return error("Classic: final write/verification failed; tag may contain new NDEF");
  return {true,"Classic 1K: NDEF written and verified"};
}
}
