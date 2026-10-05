#include "DownloadService.h"
#include "DefaultCA.h"
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <freertos/task.h>
bool DownloadService::start(const String& address,const String& path) {
  if(active)return false;
  message="";
  if(WiFi.status()!=WL_CONNECTED) { message="Connect WiFi first";return false; }
  bool tls=address.startsWith("https://");
  if((!tls&&!address.startsWith("http://"))||address.length()>512) { message="Use http:// or https:// URL";return false; }
  if(SD.exists(path)||path=="/"||path.length()>230) { message="Target must be a new file";return false; }
  ca="";
  if(tls) {
    File roots=SD.open("/config/ca.pem");
    if(roots&&roots.size()>12000) { message="CA certificate file must be <=12KB";return false; }
    ca=roots?roots.readString():String(DefaultCA::Roots);roots.close();
    if(!ca.startsWith("-----BEGIN CERTIFICATE-----")) { message="Invalid CA PEM file";return false; }
  }
  if(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<(tls?80000:30000)) {
    message="Insufficient RAM: disable Bluetooth / unused tasks";ca="";return false;
  }
  url=address;target=path;temp=path+".mtpart";
  if(SD.exists(temp)) { message="Remove stale .mtpart file first";ca="";return false; }
  file=SD.open(temp,FILE_WRITE);
  if(!file) { message="Cannot create file";ca="";return false; }
  chunks=xQueueCreate(4,sizeof(Chunk));
  received=0;cancelled=false;workerDone=false;code=0;active=true;
  if(!chunks||xTaskCreate(worker,"mt-download",12288,this,1,nullptr)!=pdPASS) {
    message="Download worker unavailable";cleanup();return false;
  }
  message="Downloading";return true;
}
void DownloadService::worker(void* context) {
  auto& self=*(DownloadService*)context;
  [&] {
  // HTTPClient decodes chunked transfer. SPI/SD remain in the main task.
  class Sink:public Stream {
    DownloadService& service;uint32_t sent=0,started=millis();
  public:
    explicit Sink(DownloadService& s):service(s) {}
    int available()override { return 0; }int read()override { return -1; }int peek()override { return -1; }
    size_t write(uint8_t byte)override { return write(&byte,1); }
    size_t write(const uint8_t* bytes,size_t count)override {
      size_t copied=0;
      while(copied<count&&!service.cancelled&&millis()-started<120000) {
        Chunk chunk;chunk.size=min(count-copied,sizeof(chunk.data));
        if(sent+chunk.size>MaxSize)return 0;
        memcpy(chunk.data,bytes+copied,chunk.size);
        while(xQueueSend(service.chunks,&chunk,pdMS_TO_TICKS(25))!=pdTRUE) {
          if(service.cancelled||millis()-started>=120000)return 0;
        }
        copied+=chunk.size;sent+=chunk.size;
      }
      return copied;
    }
  } sink(self);
  NetworkClient plain;NetworkClientSecure secure;HTTPClient http;
  if(self.ca.length())secure.setCACert(self.ca.c_str());
  NetworkClient& client=self.ca.length()?static_cast<NetworkClient&>(secure):plain;
  http.setConnectTimeout(5000);http.setTimeout(2000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int result=-1;
  if(http.begin(client,self.url)) {
    int status=http.GET();int size=http.getSize();
    if(status==200&&size<=int(MaxSize)) {
      result=http.writeToStream(&sink);
      if(size>=0&&result!=size)result=-2;
    } else result=status==200?-3:status>0?-status:status;
    http.end();
  }
  self.code=result;
  }(); // Destruct TLS/HTTP buffers before publishing completion.
  self.workerDone=true;
  vTaskDelete(nullptr);
}
void DownloadService::cleanup() {
  if(file)file.close();
  if(chunks)vQueueDelete(chunks);
  chunks=nullptr;ca="";url="";active=false;
  if(SD.exists(temp))SD.remove(temp);
}
bool DownloadService::update() {
  if(!active)return false;
  Chunk chunk;
  for(int i=0;i<4&&xQueueReceive(chunks,&chunk,0)==pdTRUE;i++) {
    if(file.write(chunk.data,chunk.size)!=chunk.size) { cancelled=true;message="SD write failed"; }
    received+=chunk.size;
  }
  if(!workerDone||uxQueueMessagesWaiting(chunks))return false;
  file.flush();file.close();
  bool ok=!cancelled&&code>=0&&!SD.exists(target)&&SD.rename(temp,target);
  if(ok)message="Saved "+String(received)+" bytes: "+target;
  else if(message=="Downloading")message=cancelled?"Download cancelled":"Download failed: "+String(code.load());
  cleanup();revision++;return true;
}
