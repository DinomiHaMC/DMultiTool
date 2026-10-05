#include "FileBridge.h"
#include "ShellParser.h"
namespace {
uint32_t updateCRC(uint32_t crc,const uint8_t* bytes,size_t size) {
  while(size--) { crc^=*bytes++;for(int i=0;i<8;i++)crc=(crc>>1)^((crc&1)?0xEDB88320:0); }
  return crc;
}
}
void FileBridge::abort() {
  if(file)file.close();
  if(receiving&&temp.length())SD.remove(temp);
  sending=receiving=waitingACK=false;outgoing="";replyOffset=0;
}
void FileBridge::reply(const String& text) { outgoing=text+"\n";replyOffset=0;status=text; }
void FileBridge::frame(const BLEFrame& input,SDModule& sd,bool notifications) {
  if(input.size<1||input.size>sizeof(input.bytes))return;
  uint8_t op=input.bytes[0];
  if(op==0x30&&notifications) {
    String text;for(int i=1;i<input.size;i++) { uint8_t c=input.bytes[i];text+=c>=32?(char)c:' '; }
    notification=text;notificationRevision++;return;
  }
  if(!active||!sd.mounted)return;
  at=millis();
  if(op==0x05&&sending&&waitingACK) { waitingACK=false;return; }
  if(op==0x04&&receiving) {
    size_t n=input.size-1;
    if(received+n>expected||file.write(input.bytes+1,n)!=n) { abort();reply("ERR write/size");return; }
    crc=updateCRC(crc,input.bytes+1,n);received+=n;reply("ACK");return;
  }
  if(op==0x06&&receiving&&input.size==5) {
    uint32_t check=uint32_t(input.bytes[1])|(uint32_t(input.bytes[2])<<8)|(uint32_t(input.bytes[3])<<16)|(uint32_t(input.bytes[4])<<24);
    if(received!=expected||check!=(crc^0xFFFFFFFF)) { abort();reply("ERR CRC/size");return; }
    file.flush();file.close();
    bool ok=!SD.exists(target)&&SD.rename(temp,target);
    if(!ok)SD.remove(temp);
    receiving=false;reply(ok?"DONE":"ERR rename");return;
  }
  if(sending||receiving||outgoing.length()) { reply("ERR busy");return; }
  String argument;for(int i=1;i<input.size;i++)argument+=(char)input.bytes[i];
  int tab=argument.indexOf('\t');String name=tab<0?argument:argument.substring(0,tab);
  std::string normalized;
  if(!MtSh::path("/",name.c_str(),normalized)) { reply("ERR path");return; }
  String path=normalized.c_str();
  if(op==0x01) {
    uint32_t offset=tab<0?0:strtoul(argument.substring(tab+1).c_str(),nullptr,10);
    if(!sd.list(path,offset)) { reply("ERR directory");return; }
    String listing;
    for(size_t i=0;i<sd.count;i++) {
      const auto& e=sd.entries[i];String line=String(e.directory?"D\t":"F\t")+String((unsigned long)e.size)+"\t"+e.name+"\n";
      if(listing.length()+line.length()>3200) { listing+="MORE\t"+String(offset+i)+"\n";break; }
      listing+=line;
      if(i+1==sd.count&&sd.truncated)listing+="MORE\t"+String(offset+sd.count)+"\n";
    }
    reply(listing+"END");
  } else if(op==0x02) {
    file=SD.open(path);
    if(!file||file.isDirectory()||file.size()>16*1024*1024) { file.close();reply("ERR file/size");return; }
    expected=file.size();received=0;crc=0xFFFFFFFF;sending=true;waitingACK=false;reply("SIZE\t"+String(expected));
  } else if(op==0x03&&tab>=0&&path!="/"&&path.length()<225) {
    char* end=nullptr;String size=argument.substring(tab+1);unsigned long length=strtoul(size.c_str(),&end,10);
    if(size.isEmpty()||*end||size[0]=='-'||length>16*1024*1024||SD.exists(path)) { reply("ERR exists/size");return; }
    target=path;temp=path+".btpart";
    if(SD.exists(temp)) { reply("ERR stale btpart");return; }
    file=SD.open(temp,FILE_WRITE);
    if(!file) { reply("ERR open");return; }
    expected=length;received=0;crc=0xFFFFFFFF;receiving=true;reply("READY");
  } else reply("ERR command");
}
void FileBridge::update(BridgeChannel& ble,SDModule& sd,bool notifications) {
  BLEFrame input;for(int i=0;i<4&&ble.receive(input);i++)frame(input,sd,notifications);
  if(!active)return;
  if(!ble.connected()) { if(sending||receiving||outgoing.length())abort();status="Waiting for PC";return; }
  if((sending||receiving)&&millis()-at>10000) { abort();reply("ERR timeout"); }
  if(millis()-lastSend<12)return;
  lastSend=millis();
  if(outgoing.length()) {
    uint8_t bytes[20]={0x20};size_t n=min(size_t(19),size_t(outgoing.length()-replyOffset));
    memcpy(bytes+1,outgoing.c_str()+replyOffset,n);
    if(ble.transmit(bytes,n+1)) { replyOffset+=n;if(replyOffset==outgoing.length())outgoing=""; }
  } else if(sending&&!waitingACK) {
    if(!file.available()) {
      uint8_t bytes[9]={0x11};uint32_t check=crc^0xFFFFFFFF;
      for(int i=0;i<4;i++) { bytes[i+1]=(received>>(i*8))&255;bytes[i+5]=(check>>(i*8))&255; }
      if(ble.transmit(bytes,sizeof(bytes))) { file.close();sending=false;status="Sent "+String(received)+" bytes"; }
    } else {
      uint8_t bytes[20]={0x10};uint32_t position=file.position();int n=file.read(bytes+1,19);
      if(n<=0) { abort();reply("ERR read");return; }
      if(ble.transmit(bytes,n+1)) { crc=updateCRC(crc,bytes+1,n);received+=n;waitingACK=true;at=millis(); }
      else file.seek(position);
    }
  }
}
