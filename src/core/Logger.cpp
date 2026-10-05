#include "Logger.h"
#include "../config.h"
#include <SD.h>
#include <stdarg.h>
Logger* Logger::instance=nullptr;
void Logger::begin(Settings& s)  {
  settings=&s;
  queue=xQueueCreate(12,192);
  instance=this;
}
void systemLog(uint8_t level,const char* scope,const char* format,...)  {
  char text[128];
  va_list args;
  va_start(args,format);
  vsnprintf(text,sizeof(text),format,args);
  va_end(args);
  if(Logger::instance)Logger::instance->log(level,scope,text);
  else Serial.printf("[%s] %s\n",scope,text);
}
void Logger::log(uint8_t level,const char* scope,const char* text)  {
  if(settings&&level<settings->logLevel)return;
  static const char* levels[]=  {
    "DEBUG","INFO","WARN","ERROR"
  };
  uint32_t sec=millis()/1000;
  char line[192];
  snprintf(line,sizeof(line),"[%02lu:%02lu:%02lu][%s][%s] %s",(unsigned long)(sec/3600),(unsigned long)(sec/60%60),(unsigned long)(sec%60),levels[min((int)level,3)],scope,text);
  if(settings&&settings->serialLog)Serial.println(line);
  if(queue)xQueueSend(queue,line,0);
}
void Logger::update(bool mounted)  {
  if(!queue)return;
  char line[192];
  for(int i=0;i<4&&xQueueReceive(queue,line,0)==pdTRUE;i++)  {
    size_t n=strlen(line);
    if(used+n+1<sizeof(buffer))  {
      memcpy(buffer+used,line,n);
      used+=n;
      buffer[used++]='\n';
    }
  }
  if(millis()-lastFlush<2000)return;
  lastFlush=millis();
  if(mounted&&used)  {
    File f=SD.open("/logs/system.log");
    bool rotate=f&&f.size()>1024*1024;
    f.close();
    if(rotate)  {
      SD.remove("/logs/system.old.log");
      SD.rename("/logs/system.log","/logs/system.old.log");
    }
    f=SD.open("/logs/system.log",FILE_APPEND);
    if(f)  {
      f.write((uint8_t*)buffer,used);
      f.close();
    }
  }
  used=0;
}
