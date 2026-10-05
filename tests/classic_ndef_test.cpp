#include "../src/services/ClassicNDEF.h"
#include "../src/services/NDEFCodec.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
struct Card:ClassicNDEF::Card {
  std::array<std::array<uint8_t,16>,64> memory{};
  std::vector<uint8_t> writes;
  int authenticated=-1,failWrite=-1,corruptWrite=-1,failedSector=-1,cancelAfter=-1;
  bool factoryNdefKey=false;
  int yields=0;
  bool stopped=false;
  Card(int first=1,int last=15,size_t offset=0) {
    for(int b=0;b<64;b++)for(int i=0;i<16;i++)memory[b][i]=(b+i)&255;
    memory[1].fill(0);memory[2].fill(0);memory[1][1]=1;
    memory[3][9]=0xC1;
    for(int s=first;s<=last;s++) {
      size_t pos=s*2;memory[1+pos/16][pos%16]=3;memory[1+pos/16][pos%16+1]=0xE1;
      auto& t=memory[s*4+3];t[6]=0x7F;t[7]=7;t[8]=0x88;t[9]=0x40;
      for(int b=0;b<3;b++)memory[s*4+b].fill(0);
    }
    auto original=NDEF::encode("Phone NDEF",false);
    for(size_t i=0;i<original.size();i++) {
      size_t index=offset+i;memory[first*4+(index/48)*4+(index%48)/16][index%16]=original[i];
    }
    checksum();
  }
  void checksum() {
    uint8_t mad[32];memcpy(mad,memory[1].data(),16);memcpy(mad+16,memory[2].data(),16);
    memory[1][0]=ClassicNDEF::madCRC(mad+1,31);
  }
  bool authenticate(uint8_t sector,const uint8_t key[6])override {
    static const uint8_t mad[]={0xA0,0xA1,0xA2,0xA3,0xA4,0xA5},ndef[]={0xD3,0xF7,0xD3,0xF7,0xD3,0xF7},factory[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    if(sector&&factoryNdefKey&&memcmp(key,ndef,6)==0)return false;
    if(sector)assert(memcmp(key,ndef,6)==0||memcmp(key,factory,6)==0);
    else assert(memcmp(key,mad,6)==0||memcmp(key,factory,6)==0);
    if(sector==failedSector)return false;
    authenticated=sector;return true;
  }
  bool read(uint8_t block,uint8_t data[16])override {
    assert(block<64&&authenticated==block/4);memcpy(data,memory[block].data(),16);return true;
  }
  bool write(uint8_t block,const uint8_t data[16])override {
    assert(block>=4&&block<64&&block%4!=3&&authenticated==block/4);
    size_t pos=(block/4)*2;assert(memory[1+pos/16][pos%16]==3&&memory[1+pos/16][pos%16+1]==0xE1);
    int n=writes.size();writes.push_back(block);
    if(n==failWrite)return false;
    memcpy(memory[block].data(),data,16);if(n==corruptWrite)memory[block][5]^=1;
    return true;
  }
  bool cancelled()const override { return stopped; }
  void yield()override { if(++yields==cancelAfter)stopped=true; }
};
int main() {
  const uint8_t sample[]={1,1,8,1,8,1,8,0,0,0,0,0,0,4,0,3,16,3,16,2,16,2,16,0,0,0,0,0,0,17,48};
  assert(ClassicNDEF::madCRC(sample,sizeof(sample))==0x89);
  Card card;assert(card.memory[1][0]==0x14);
  auto read=ClassicNDEF::operate(card,false,"",false);assert(read.ok&&read.text=="Text: Phone NDEF\n");
  Card factory;factory.factoryNdefKey=true;
  assert(ClassicNDEF::operate(factory,false,"",false).ok);
  assert(ClassicNDEF::operate(factory,true,"Factory key NDEF",false).ok);
  Card laterLocked;laterLocked.failedSector=6;
  assert(ClassicNDEF::operate(laterLocked,false,"",false).ok);
  auto before=card.memory;
  std::string message(100,'x');auto result=ClassicNDEF::operate(card,true,message,false);assert(result.ok);
  read=ClassicNDEF::operate(card,false,"",false);assert(read.ok&&read.text=="Text: "+message+"\n");
  for(int b=0;b<64;b++)if(b<4||b%4==3)assert(card.memory[b]==before[b]);
  for(size_t offset:{size_t(0),size_t(2),size_t(14),size_t(15)}) {
    Card c(2,5,offset);before=c.memory;
    assert(ClassicNDEF::operate(c,true,"https://example.org/hello",true).ok);
    assert(ClassicNDEF::operate(c,false,"",false).text=="URI: https://example.org/hello\n");
    for(int b=0;b<64;b++)if(b<8||b>22||b%4==3)assert(c.memory[b]==before[b]);
  }
  Card small(1,1);assert(!ClassicNDEF::operate(small,true,message,false).ok&&small.writes.empty());
  Card bad;bad.memory[1][0]^=1;assert(!ClassicNDEF::operate(bad,true,"test",false).ok&&bad.writes.empty());
  Card locked;locked.memory[7][9]=0x43;
  assert(ClassicNDEF::operate(locked,false,"",false).ok);
  assert(!ClassicNDEF::operate(locked,true,"test",false).ok&&locked.writes.empty());
  Card readonly;readonly.memory[7][6]=7;readonly.memory[7][7]=0x8F;readonly.memory[7][8]=0x0F;readonly.memory[7][9]=0x43;
  assert(ClassicNDEF::operate(readonly,false,"",false).ok);
  assert(!ClassicNDEF::operate(readonly,true,"test",false).ok&&readonly.writes.empty());
  Card access;access.memory[7][6]^=1;assert(!ClassicNDEF::operate(access,true,"test",false).ok&&access.writes.empty());
  Card auth;auth.failedSector=2;assert(!ClassicNDEF::operate(auth,true,message,false).ok&&auth.writes.empty());
  Card missing;missing.memory[3][9]=0x69;assert(!ClassicNDEF::operate(missing,true,"test",false).ok&&missing.writes.empty());
  Card interrupted;interrupted.failWrite=1;
  assert(!ClassicNDEF::operate(interrupted,true,message,false).ok&&interrupted.memory[4][1]==0);
  Card corrupt;corrupt.corruptWrite=1;
  assert(!ClassicNDEF::operate(corrupt,true,message,false).ok&&corrupt.memory[4][1]==0);
  Card cancelled;cancelled.cancelAfter=10;
  assert(!ClassicNDEF::operate(cancelled,true,message,false).ok&&cancelled.memory[4][1]==0);
  std::cout<<"Classic NDEF tests passed: MAD CRC, phone read/write, only needed sectors, multiple sectors, URI, offset/block boundary, metadata preservation, preflight/auth/read-only/size, cancellation and read-back failure\n";
}
