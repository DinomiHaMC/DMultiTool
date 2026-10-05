#include "NDEFCodec.h"
namespace NDEF  {
  std::vector<uint8_t> encode(const std::string& value,bool uri)  {
    if(value.empty()||value.size()>100)return  {
    };
    std::vector<uint8_t> payload;
    if(uri)payload=  {
      0
    };
    else payload=  {
      2,'e','n'
    };
    payload.insert(payload.end(),value.begin(),value.end());
    std::vector<uint8_t> bytes=  {
      3,(uint8_t)(4+payload.size()),0xD1,1,(uint8_t)payload.size(),(uint8_t)(uri?'U':'T')
    };
    bytes.insert(bytes.end(),payload.begin(),payload.end());
    bytes.push_back(0xFE);
    while(bytes.size()%4)bytes.push_back(0);
    return bytes;
  }
  bool decode(const uint8_t* data,size_t length,std::string& out)  {
    out.clear();
    size_t pos=0,end=0;
    bool found=false;
    while(pos<length)  {
      uint8_t tag=data[pos++];
      if(tag==0)continue;
      if(tag==0xFE)break;
      if(pos==length)return false;
      size_t n=data[pos++];
      if(n==255)  {
        if(pos+2>length)return false;
        n=(data[pos]<<8)|data[pos+1];
        pos+=2;
      }
      if(n>length-pos)return false;
      if(tag==3)  {
        end=pos+n;
        found=true;
        break;
      }
      pos+=n;
    }
    if(!found)return false;
    bool first=true,ended=false;
    while(pos<end)  {
      if(end-pos<3)return false;
      uint8_t flags=data[pos++];
      if(flags&0x20)return false;
      if(first&&!(flags&0x80))return false;
      if(!first&&(flags&0x80))return false;
      first=false;
      size_t typeLength=data[pos++],payload=0;
      if(flags&0x10)payload=data[pos++];
      else  {
        if(end-pos<4)return false;
        for(int i=0;i<4;i++)payload=(payload<<8)|data[pos++];
      }
      size_t id=0;
      if(flags&8)  {
        if(pos==end)return false;
        id=data[pos++];
      }
      if(typeLength>end-pos)return false;
      const uint8_t* type=data+pos;
      pos+=typeLength;
      if(id>end-pos)return false;
      pos+=id;
      if(payload>end-pos)return false;
      const uint8_t* p=data+pos;
      uint8_t tnf=flags&7;
      if(tnf==1&&typeLength==1&&type[0]=='T')  {
        if(!payload||(p[0]&0x80))return false;
        size_t lang=p[0]&63;
        if(lang+1>payload)return false;
        out+="Text: ";
        out.append((const char*)p+lang+1,payload-lang-1);
      }
      else if(tnf==1&&typeLength==1&&type[0]=='U')  {
        if(!payload)return false;
        static const char* prefixes[]=  {
          "","http://www.","https://www.","http://","https://","tel:","mailto:"
        };
        out+="URI: ";
        if(p[0]<7)out+=prefixes[p[0]];
        else out+="[prefix "+std::to_string(p[0])+"] ";
        out.append((const char*)p+1,payload-1);
      }
      else  {
        out+=tnf==2?"MIME: ":"Record: ";
        out.append((const char*)type,typeLength);
        out+="\n";
        for(size_t i=0;i<payload&&i<120;i++)  {
          char c=p[i];
          out+=(c>=32&&c<127)?c:'.';
        }
        if(payload>120)out+=" [truncated]";
      }
      out+="\n";
      pos+=payload;
      if(flags&0x40)  {
        ended=true;
        break;
      }
    }
    return ended&&pos==end;
  }
}
