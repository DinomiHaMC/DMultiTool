#include "SDModule.h"
#include "../pins.h"
void SDModule::begin()  {
  mounted=SD.begin(Pins::SD_CS,SPI,Config::SdFrequency) && SD.cardType()!=CARD_NONE;
  if(mounted)  {
    for(const char* p:  {
      "/nfc","/ir","/ir/presets","/scripts","/python","/wifi","/captures","/logs","/config","/apps"
    }
    ) if(!SD.exists(p)&&!SD.mkdir(p))LOG_ERROR("SD","mkdir %s failed",p);
  }
  LOG_INFO("BOOT","SD %s",mounted?"OK":"FAIL");
}
bool SDModule::list(const String& path,uint32_t offset)  {
  count=0;
  truncated=false;
  if(!mounted)return false;
  File dir=SD.open(path);
  if(!dir || !dir.isDirectory())return false;
  uint32_t skipped=0;
  while(true)  {
    File f=dir.openNextFile();
    if(!f)break;
    if(skipped++<offset)  {
      f.close();
      continue;
    }
    if(count==Config::MaxEntries)  {
      truncated=true;
      f.close();
      break;
    }
    auto& e=entries[count++];
    String n=f.name();
    int slash=n.lastIndexOf('/');
    n=n.substring(slash+1);
    n.toCharArray(e.name,sizeof(e.name));
    e.directory=f.isDirectory();
    e.size=f.size();
    f.close();
  }
  dir.close();
  for(size_t i=0;i<count;i++)for(size_t j=i+1;j<count;j++)if((entries[j].directory&&!entries[i].directory)||(entries[j].directory==entries[i].directory&&strcasecmp(entries[j].name,entries[i].name)<0))  {
    FileEntry t=entries[i];
    entries[i]=entries[j];
    entries[j]=t;
  }
  return true;
}
bool SDModule::removeFile(const String& p)  {
  if(!mounted)return false;
  File f=SD.open(p);
  if(!f)return false;
  bool dir=f.isDirectory();
  f.close();
  bool ok=!dir&&SD.remove(p);
  LOG_INFO("SD","Delete %s: %s",p.c_str(),ok?"OK":"FAIL");
  return ok;
}
bool SDModule::writeNew(const String& p,const String& content)  {
  if(!mounted||SD.exists(p))return false;
  File f=SD.open(p,FILE_WRITE);
  if(!f)return false;
  bool ok=f.print(content)==content.length();
  f.close();
  if(!ok)SD.remove(p);
  return ok;
}
bool SDModule::readPage(const String& p,uint32_t offset,String& out,uint32_t& next)  {
  out="";
  next=0;
  if(!mounted)return false;
  File f=SD.open(p);
  if(!f||f.isDirectory()||!f.seek(offset))return false;
  out.reserve(512);
  int lines=0,col=0;
  while(f.available()&&lines<15)  {
    char c=f.read();
    if(c=='\r')continue;
    if(c=='\n')  {
      out+='\n';
      lines++;
      col=0;
    }
    else  {
      out+=(c>=32&&c<=126)?c:'.';
      if(++col>=32)  {
        out+='\n';
        lines++;
        col=0;
      }
    }
  }
  if(f.available())next=f.position();
  f.close();
  return true;
}
