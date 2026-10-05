#include "../src/modules/PN532Polling.h"
#include <array>
#include <cassert>
#include <deque>
#include <iostream>
#include <vector>
static bool classicCandidate(uint16_t atqa,uint8_t sak) {
  return (sak&0x08)!=0||(sak==0&&atqa==0x0004);
}
struct Bus {
  std::deque<uint8_t> pending;
  std::deque<uint8_t> received;
  size_t readLimit=10;
  size_t requestFrom(uint8_t address,size_t count) {
    assert(address==0x24&&count==10);
    while(!pending.empty()&&received.size()<readLimit) {
      received.push_back(pending.front());pending.pop_front();
    }
    return received.size();
  }
  int read() { assert(!received.empty());int value=received.front();received.pop_front();return value; }
};
struct Reader {
  Bus& bus;
  bool ack=true,corrupt=false,present=true;
  uint8_t reportedLength=7;
  uint16_t latency=180;
  bool sendCommandCheckAck(uint8_t* command,uint8_t count,uint16_t timeout) {
    assert(count==5&&command[0]==0x32&&command[1]==5);
    assert(command[2]==0xFF&&command[3]==1&&command[4]!=0xFF);
    assert(timeout!=0);
    if(!ack)return false;
    bus.pending={1,0,0,0xFF,2,0xFE,0xD5,0x33,0xF8,0};
    if(corrupt)bus.pending[8]^=1;
    return true;
  }
  bool readPassiveTargetID(uint8_t baud,uint8_t* uid,uint8_t* length,uint16_t timeout) {
    assert(baud==0);
    if(!bus.pending.empty()||latency>timeout||!present)return false;
    *length=reportedLength;
    for(unsigned i=0;i<reportedLength;i++)uid[i]=i+1;
    return true;
  }
};
struct TargetBus {
  std::vector<uint8_t> reply;
  size_t cursor=0;
  size_t requestFrom(uint8_t address,size_t count) { assert(address==0x24&&count==64);cursor=0;return reply.size(); }
  int read() { return reply[cursor++]; }
};
struct TargetReader {
  bool sendCommandCheckAck(uint8_t* command,uint8_t count,uint16_t timeout) {
    assert(count==3&&command[0]==0x4A&&command[1]==1&&command[2]==0&&timeout==500);return true;
  }
};
static std::vector<uint8_t> targetFrame(uint8_t length,uint8_t sak) {
  std::vector<uint8_t> frame={1,0,0,0xFF,(uint8_t)(8+length),(uint8_t)(0-(8+length)),0xD5,0x4B,1,1,0,4,sak,length};
  for(int i=0;i<length;i++)frame.push_back(i+1);
  uint8_t sum=0;for(size_t i=6;i<frame.size();i++)sum+=frame[i];
  frame.push_back(0-sum);frame.push_back(0);return frame;
}
int main() {
  Bus bus;Reader reader{bus};uint8_t uid[7]{},length=0;
  // Reproduce a ready-but-unread RFConfiguration response poisoning a new read.
  uint8_t config[]={0x32,5,0xFF,1,2};
  assert(reader.sendCommandCheckAck(config,5,500));
  assert(!reader.readPassiveTargetID(0,uid,&length,500));
  bus.pending.clear();
  assert(PN532Polling::configure(reader,bus,0x24));
  assert(bus.pending.empty()&&bus.received.empty());
  // A tag response after 180ms is missed by the previous 80ms timeout.
  assert(!reader.readPassiveTargetID(0,uid,&length,80));
  assert(PN532Polling::readTag(reader,0,uid,length));
  assert(length==7&&uid[0]==1&&uid[6]==7);
  reader.reportedLength=4;assert(PN532Polling::readTag(reader,0,uid,length)&&length==4);
  reader.present=false;assert(!PN532Polling::readTag(reader,0,uid,length)&&length==0);
  reader.present=true;reader.latency=600;assert(!PN532Polling::readTag(reader,0,uid,length));
  reader.latency=180;reader.reportedLength=250;
  std::array<uint8_t,9> bounded{};bounded.fill(0xAB);
  assert(!PN532Polling::readTag(reader,0,bounded.data()+1,length));
  for(auto byte:bounded)assert(byte==0xAB);
  reader.corrupt=true;assert(!PN532Polling::configure(reader,bus,0x24));
  reader.corrupt=false;bus.readLimit=9;assert(!PN532Polling::configure(reader,bus,0x24));
  bus.pending.clear();bus.received.clear();reader.ack=false;
  assert(!PN532Polling::configure(reader,bus,0x24));
  TargetBus target;TargetReader selector;uint16_t atqa=0;uint8_t sak=0;
  for(int len:{4,7}) {
    target.reply=targetFrame(len,8);
    assert(PN532Polling::selectTag(selector,target,0x24,0,uid,length,atqa,sak));
    assert(length==len&&atqa==4&&sak==8&&uid[len-1]==len);
    assert(classicCandidate(atqa,sak));
  }
  target.reply=targetFrame(4,0);
  assert(PN532Polling::selectTag(selector,target,0x24,0,uid,length,atqa,sak));
  assert(classicCandidate(atqa,sak));
  // Type 2 tags commonly have ATQA 0044 with SAK 00: do not send Classic
  // authentication commands to them.
  target.reply=targetFrame(4,0);target.reply[10]=0x44;
  uint8_t sum=0;for(size_t i=6;i<target.reply.size()-2;i++)sum+=target.reply[i];
  target.reply[target.reply.size()-2]=0-sum;
  assert(PN532Polling::selectTag(selector,target,0x24,0,uid,length,atqa,sak));
  assert(!classicCandidate(atqa,sak));
  target.reply.back()=1;assert(!PN532Polling::selectTag(selector,target,0x24,0,uid,length,atqa,sak)&&length==0);
  target.reply=targetFrame(7,0);target.reply[target.reply.size()-2]^=1;
  assert(!PN532Polling::selectTag(selector,target,0x24,0,uid,length,atqa,sak));
  target.reply=targetFrame(7,0);target.reply.resize(15);
  assert(!PN532Polling::selectTag(selector,target,0x24,0,uid,length,atqa,sak));
  target.reply=targetFrame(8,0);
  assert(!PN532Polling::selectTag(selector,target,0x24,0,uid,length,atqa,sak));
  target.reply={1,0,0,0xFF,3,0xFD,0xD5,0x4B,0,0xE0,0};
  assert(!PN532Polling::selectTag(selector,target,0x24,0,uid,length,atqa,sak));
  std::cout<<"NFC polling tests passed: full RF reply, delayed/no tag, bounded timeout, UID bounds, corrupt/truncated reply, ACK failure, SAK/ATQA/full target frame\n";
}
