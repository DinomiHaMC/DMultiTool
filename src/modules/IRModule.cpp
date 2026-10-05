#define DISABLE_CODE_FOR_RECEIVER
#include <IRremote.hpp>
#include "IRModule.h"
#include "../pins.h"
#include "../config.h"
void IRModule::begin()  {
  IrSender.begin(Pins::IR_TX);
  jobs=xQueueCreate(1,sizeof(IRJob));
  done=xQueueCreate(1,sizeof(bool));
  ready=jobs&&done&&xTaskCreate(worker,"ir",3072,this,1,nullptr)==pdPASS;
  LOG_INFO("BOOT","IR TX %s",ready?"ready":"FAIL");
}
void IRModule::worker(void* p)  {
  auto& s=*static_cast<IRModule*>(p);
  IRJob job;
  for(;;)if(xQueueReceive(s.jobs,&job,portMAX_DELAY)==pdTRUE)  {
    if(job.length)IrSender.sendRaw(job.timings,job.length,job.khz);
    else IrSender.sendNEC(job.signal.address,job.signal.command,job.signal.repeats);
    bool ok=true;
    xQueueOverwrite(s.done,&ok);
  }
}
bool IRModule::send(IRSignal s)  {
  if(!ready||busy||s.repeats>5)return false;
  IRJob job;
  job.signal=s;
  busy=xQueueSend(jobs,&job,0)==pdTRUE;
  return busy;
}
bool IRModule::update()  {
  bool ok;
  if(done&&xQueueReceive(done,&ok,0)==pdTRUE)  {
    busy=false;
    LOG_INFO("IR","NEC sent");
    return true;
  }
  return false;
}
bool IRModule::parse(const String& text,IRSignal& s)  {
  unsigned a,c,r;
  char tail;
  if(sscanf(text.c_str(),"NEC %x %x %u %c",&a,&c,&r,&tail)!=3||a>65535||c>255||r>5)return false;
  s.address=a;
  s.command=c;
  s.repeats=r;
  return true;
}
bool IRModule::sendRaw(const uint16_t* timings,size_t count,uint8_t khz)  {
  if(!ready||busy||!count||count>256||khz<30||khz>60)return false;
  uint32_t total=0;
  IRJob job;
  job.length=count;
  job.khz=khz;
  for(size_t i=0;i<count;i++)  {
    if(!timings[i]||timings[i]>20000)return false;
    total+=timings[i];
    job.timings[i]=timings[i];
  }
  if(total>200000)return false;
  busy=xQueueSend(jobs,&job,0)==pdTRUE;
  return busy;
}
