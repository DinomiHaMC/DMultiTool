#include "CaptureService.h"
#include "../config.h"
CaptureService* CaptureService::instance=nullptr;
void CaptureService::receive(void* buf,wifi_promiscuous_pkt_type_t type)  {
  auto* s=instance;
  if(!s||!s->active||type!=WIFI_PKT_MGMT)return;
  auto* p=(wifi_promiscuous_pkt_t*)buf;
  if(p->rx_ctrl.rx_state)return;
  int n=p->rx_ctrl.sig_len-4;
  if(n<24||memcmp(p->payload+16,s->filterBssid,6))return;
  CaptureFrame frame;
  frame.micros=micros()-s->originMicros;
  frame.length=min(n,256);
  frame.original=n;
  memcpy(frame.bytes,p->payload,frame.length);
  if(xQueueSend(s->queue,&frame,0)==pdTRUE)s->packets++;
  else s->dropped++;
}
bool CaptureService::start(const String& path,const String& bssid,uint8_t channel,bool writeFile)  {
  if(active||channel<1||channel>13)return false;
  unsigned mac[6];
  if(sscanf(bssid.c_str(),"%x:%x:%x:%x:%x:%x",mac,mac+1,mac+2,mac+3,mac+4,mac+5)!=6)return false;
  for(int i=0;i<6;i++)  {
    if(mac[i]>255)return false;
    filterBssid[i]=mac[i];
  }
  if(!queue)queue=xQueueCreate(12,sizeof(CaptureFrame));
  if(!queue)return false;
  xQueueReset(queue);
  if(writeFile)  {
    if(SD.exists(path))return false;
    file=SD.open(path,FILE_WRITE);
    if(!file)return false;
    uint32_t header[]=  {
      0xa1b2c3d4,0x00040002,0,0,256,105
    };
    if(file.write((uint8_t*)header,sizeof(header))!=sizeof(header))  {
      file.close();
      return false;
    }
  }
  else file.close();
  filename=path;
  packets=0;
  dropped=0;
  instance=this;
  wifi_promiscuous_filter_t filter=  {
    WIFI_PROMIS_FILTER_MASK_MGMT
  };
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(receive);
  if(WiFi.status()!=WL_CONNECTED&&esp_wifi_set_channel(channel,WIFI_SECOND_CHAN_NONE)!=ESP_OK)  {
    file.close();
    return false;
  }
  started=millis();
  originMicros=micros();
  active=true;
  if(esp_wifi_set_promiscuous(true)!=ESP_OK)  {
    active=false;
    file.close();
    return false;
  }
  return true;
}
void CaptureService::stop()  {
  active=false;
  esp_wifi_set_promiscuous(false);
  if(file)  {
    CaptureFrame frame;
    for(int i=0;i<12&&xQueueReceive(queue,&frame,0)==pdTRUE;i++)  {
      uint32_t header[]=  {
        frame.micros/1000000,frame.micros%1000000,frame.length,frame.original
      };
      if(file.write((uint8_t*)header,16)!=16||file.write(frame.bytes,frame.length)!=frame.length)break;
    }
    file.flush();
    file.close();
  }
  LOG_INFO("CAPTURE","Stopped %lu packets",(unsigned long)packets.load());
}
void CaptureService::update()  {
  if(!active)return;
  CaptureFrame frame;
  for(int i=0;i<3&&xQueueReceive(queue,&frame,0)==pdTRUE;i++)  {
    if(file)  {
      uint32_t header[]=  {
        frame.micros/1000000,frame.micros%1000000,frame.length,frame.original
      };
      if(file.write((uint8_t*)header,16)!=16||file.write(frame.bytes,frame.length)!=frame.length)  {
        stop();
        return;
      }
    }
  }
  if(millis()-started>30000||(file&&file.size()>1024*1024))stop();
}
