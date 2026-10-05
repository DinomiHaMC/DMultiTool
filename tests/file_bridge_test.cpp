#include "../src/services/FileBridge.h"
#include <cassert>
#include <deque>
#include <iostream>
#include <cstring>
bool SDModule::list(const String& path,uint32_t offset) {
  count=0;truncated=false;
  if(path!="/")return false;
  for(const auto& pair:SD.nodes) {
    if(offset) { offset--;continue; }
    if(count==Config::MaxEntries) { truncated=true;break; }
    auto& entry=entries[count++];snprintf(entry.name,sizeof(entry.name),"%s",pair.first.c_str()+1);
    entry.size=pair.second->bytes.size();entry.directory=pair.second->directory;
  }
  return true;
}
struct Link:BridgeChannel {
  bool online=true;std::deque<BLEFrame> incoming;std::deque<std::vector<uint8_t>> outgoing;
  bool connected()const override { return online; }
  bool receive(BLEFrame& frame)override { if(incoming.empty())return false;frame=incoming.front();incoming.pop_front();return true; }
  bool transmit(const uint8_t* bytes,size_t count)override { assert(count<=20);outgoing.emplace_back(bytes,bytes+count);return true; }
  void send(uint8_t opcode,const std::string& text="") {
    assert(text.size()<200);BLEFrame frame;frame.size=text.size()+1;frame.bytes[0]=opcode;memcpy(frame.bytes+1,text.data(),text.size());incoming.push_back(frame);
  }
};
int main() {
  FileBridge bridge;SDModule sd;sd.mounted=true;Link link;bridge.enable(true);
  auto tick=[&]{testUiMillis+=20;bridge.update(link,sd,true);};
  auto text=[&] {
    std::string result;
    for(int n=0;n<250;n++) {
      tick();while(!link.outgoing.empty()) { auto bytes=link.outgoing.front();link.outgoing.pop_front();assert(bytes[0]==0x20);result.append(bytes.begin()+1,bytes.end()); }
      if(!result.empty()&&result.back()=='\n')return result;
    }
    assert(false);return result;
  };
  auto crc=[&](uint32_t value) { std::string bytes;for(int i=0;i<4;i++)bytes+=char(value>>(8*i));link.send(6,bytes); };
  link.send(3,"/nine.txt\t9");assert(text()=="READY\n");assert(!SD.exists("/nine.txt"));
  link.send(4,"123456789");assert(text()=="ACK\n");crc(0xCBF43926);assert(text()=="DONE\n");assert(SD.exists("/nine.txt")&&!SD.exists("/nine.txt.btpart"));
  link.send(2,"/nine.txt");assert(text()=="SIZE\t9\n");tick();assert(link.outgoing.size()==1);
  auto data=link.outgoing.front();link.outgoing.pop_front();assert(data[0]==0x10&&std::string(data.begin()+1,data.end())=="123456789");
  link.send(5);tick();auto end=link.outgoing.front();link.outgoing.pop_front();assert(end[0]==0x11&&end[1]==9&&end[5]==0x26&&end[6]==0x39&&end[7]==0xF4&&end[8]==0xCB);
  link.send(3,"/bad.txt\t9");assert(text()=="READY\n");link.send(4,"123456789");assert(text()=="ACK\n");crc(1);assert(text()=="ERR CRC/size\n");assert(!SD.exists("/bad.txt")&&!SD.exists("/bad.txt.btpart"));
  link.send(3,"/overshoot\t2");assert(text()=="READY\n");link.send(4,"123");assert(text()=="ERR write/size\n");assert(!SD.exists("/overshoot.btpart"));
  link.send(3,"/empty\t0");assert(text()=="READY\n");crc(0);assert(text()=="DONE\n");assert(SD.exists("/empty"));
  link.send(3,"/nine.txt\t9");assert(text()=="ERR exists/size\n");
  link.send(3,"/race\t0");assert(text()=="READY\n");
  SD.nodes["/race"]=std::make_shared<Node>();SD.nodes["/race"]->bytes={99};
  crc(0);assert(text()=="ERR rename\n");assert(SD.nodes["/race"]->bytes==std::vector<uint8_t>{99}&&!SD.exists("/race.btpart"));
  link.send(3,"/cancel\t9");assert(text()=="READY\n");link.online=false;tick();assert(!SD.exists("/cancel.btpart"));link.online=true;
  link.send(3,"/full\t9");assert(text()=="READY\n");failWrites=true;link.send(4,"123456789");assert(text()=="ERR write/size\n");failWrites=false;
  link.send(1,"/");assert(text().find("nine.txt")!=std::string::npos);
  link.send(0x30,"App\tMessage");tick();assert(bridge.notification=="App Message"&&bridge.notificationRevision==1);
  bridge.enable(false);link.send(3,"/disabled\t0");tick();assert(link.outgoing.empty()&&!SD.exists("/disabled.btpart"));
  std::cout<<"File bridge passed: actual firmware upload/download/CRC, empty files, no overwrite, overshoot, SD failure, disconnect cleanup, list, notifications\n";
}
