#include "NetworkTools.h"
#include "../config.h"
#include <HTTPClient.h>
#include <ping/ping_sock.h>
#include <freertos/semphr.h>
struct PingWait  {
  SemaphoreHandle_t end;
  uint32_t ms=0;
  bool success=false;
};
static bool pingOnce(IPAddress ip,uint32_t& ms)  {
  PingWait wait  {
    xSemaphoreCreateBinary()
  };
  if(!wait.end)return false;
  esp_ping_config_t cfg=ESP_PING_DEFAULT_CONFIG();
  cfg.count=1;
  cfg.interval_ms=10;
  cfg.timeout_ms=250;
  ipaddr_aton(ip.toString().c_str(),&cfg.target_addr);
  esp_ping_callbacks_t cb=  {
  };
  cb.cb_args=&wait;
  cb.on_ping_success=[](esp_ping_handle_t h,void* p)  {
    auto* w=(PingWait*)p;
    w->success=true;
    esp_ping_get_profile(h,ESP_PING_PROF_TIMEGAP,&w->ms,sizeof(w->ms));
  };
  cb.on_ping_end=[](esp_ping_handle_t,void* p)  {
    xSemaphoreGive(((PingWait*)p)->end);
  };
  esp_ping_handle_t h=nullptr;
  bool ok=esp_ping_new_session(&cfg,&cb,&h)==ESP_OK;
  if(ok)  {
    if(esp_ping_start(h)==ESP_OK)xSemaphoreTake(wait.end,portMAX_DELAY);
    esp_ping_stop(h);
    esp_ping_delete_session(h);
  }
  ms=wait.ms;
  vSemaphoreDelete(wait.end);
  return ok&&wait.success;
}
void NetworkTools::begin()  {
  // No idle worker/queues: allocate them only for a diagnostic operation.
  ready=true;
}
bool NetworkTools::start(const NetJob& job)  {
  if(!ready||busy||WiFi.status()!=WL_CONNECTED)return false;
  busy=true;
  cancelled=false;
  progress=0;
  output="";
  pending=job;
  workerDone=false;
  results=xQueueCreate(1,sizeof(NetResult));
  if(!results||xTaskCreate(worker,"net-tools",12288,this,1,nullptr)!=pdPASS)  {
    if(results)vQueueDelete(results);
    results=nullptr;
    busy=false;
    return false;
  }
  return true;
}
bool NetworkTools::update()  {
  NetResult result;
  if(workerDone.load()&&results&&xQueueReceive(results,&result,0)==pdTRUE)  {
    output=result.text;
    vQueueDelete(results);
    results=nullptr;
    workerDone=false;
    busy=false;
    revision++;
    LOG_INFO("NET","Diagnostic finished");
    return true;
  }
  return false;
}
void NetworkTools::worker(void* p)  {
  auto& self=*(NetworkTools*)p;
  NetResult result;
  self.execute(self.pending,result);
  xQueueOverwrite(self.results,&result);
  // After publication, the worker never accesses its queues or job again.
  self.workerDone=true;
  vTaskDelete(nullptr);
}
void NetworkTools::execute(const NetJob& j,NetResult& result)  {
  String text;
  text.reserve(3500);
  IPAddress ip;
  if(j.operation==NetOperation::HTTP)  {
    String url=j.host;
    if(!url.startsWith("http://"))  {
      text="Use http:// (TLS trust not configured)";
    }
    else  {
      WiFiClient client;
      HTTPClient http;
      http.setConnectTimeout(2000);
      http.setTimeout(1500);
      if(http.begin(client,url))  {
        int code=http.GET();
        text="HTTP status: "+String(code)+"\n";
        if(code>0)  {
          class LimitedBody:public Stream  {
            String& text;
            std::atomic<bool>& cancel;
            size_t written=0;
            public:LimitedBody(String& out,std::atomic<bool>& stop):text(out),cancel(stop)  {
            }
            int available()override  {
              return 0;
            }
            int read()override  {
              return -1;
            }
            int peek()override  {
              return -1;
            }
            size_t write(uint8_t byte)override  {
              if(cancel||written==2048)return 0;
              text+=(byte=='\n'||(byte>=32&&byte<127))?(char)byte:'.';
              written++;
              return 1;
            }
            size_t write(const uint8_t* bytes,size_t count)override  {
              size_t n=0;
              for(;n<count;n++)if(!write(bytes[n]))break;
              return n;
            }
          }
          body(text,cancelled);
          http.writeToStream(&body);
          text+="\n[body limited to ~2 KB]";
        }
        else text+=http.errorToString(code);
        http.end();
      }
      else text="HTTP initialization failed";
    }
  }
  else if(j.operation==NetOperation::Listen)  {
    WiFiServer server(j.port);
    server.begin();
    text="Listen "+WiFi.localIP().toString()+":"+String(j.port)+"\n";
    uint32_t start=millis();
    WiFiClient client;
    bool received=false;
    while(!cancelled&&millis()-start<15000)  {
      if(!client)client=server.accept();
      if(client)  {
        while(client.available()&&text.length()<2048)  {
          char c=client.read();
          received=true;
          text+=(c=='\n'||(c>=32&&c<127))?c:'.';
        }
        if(client.available()==0&&received)  {
          size_t written=client.println(j.text);
          text+=written?"\nReply sent":"\nReply failed";
          break;
        }
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    client.stop();
    server.end();
    text+="\nListener stopped (15s limit)";
  }
  else if(!ip.fromString(j.host)&&!WiFi.hostByName(j.host,ip))text="DNS lookup failed";
  else if(j.operation==NetOperation::DNS)text=String(j.host)+"\n"+ip.toString();
  else if(j.operation==NetOperation::Ping)  {
    int replies=0;
    for(int n=0;n<4&&!cancelled;n++)  {
      uint32_t ms;
      bool ok=pingOnce(ip,ms);
      if(ok)replies++;
      text+="#"+String(n+1)+" "+(ok?String(ms)+" ms":"timeout")+"\n";
      progress=n+1;
      vTaskDelay(pdMS_TO_TICKS(20));
    }
    text+="Loss: "+String((4-replies)*25)+"%";
  }
  else if(j.operation==NetOperation::Hosts)  {
    uint32_t base=(uint32_t)ip,mask=(uint32_t)WiFi.subnetMask(),local=(uint32_t)WiFi.localIP();
    if((base&mask)!=(local&mask))text="Start IP must be in current subnet";
    else  {
      for(int n=0;n<64&&!cancelled;n++)  {
        IPAddress target=ip;
        uint16_t octet=(uint16_t)ip[3]+n;
        if(octet>254)break;
        target[3]=octet;
        if(((uint32_t)target&mask)!=(local&mask))break;
        uint32_t ms;
        if(pingOnce(target,ms))text+=target.toString()+" "+String(ms)+" ms\n";
        progress=n+1;
        vTaskDelay(1);
      }
      text+="ICMP discovery (max 64 addresses)\nNo reply does not mean offline.\nHostname/MAC: unavailable";
    }
  }
  else if(j.operation==NetOperation::Ports)  {
    if(!j.port||j.endPort<j.port||j.endPort-j.port>=128)text="Range must contain 1..128 ports";
    else  {
      for(uint32_t port=j.port;port<=j.endPort&&!cancelled;port++)  {
        WiFiClient client;
        if(client.connect(ip,port,150))text+=String(port)+" open\n";
        client.stop();
        progress=port-j.port+1;
        vTaskDelay(1);
      }
      text+="Done (TCP connect, 150ms timeout)";
    }
  }
  else if(j.operation==NetOperation::TCP)  {
    WiFiClient client;
    if(client.connect(ip,j.port,1500))  {
      client.println(j.text);
      text="Sent; response:\n";
      uint32_t start=millis();
      while(client.connected()&&!cancelled&&millis()-start<5000&&text.length()<2048)  {
        while(client.available()&&text.length()<2048)  {
          char c=client.read();
          text+=(c=='\n'||(c>=32&&c<127))?c:'.';
        }
        vTaskDelay(pdMS_TO_TICKS(10));
      }
      client.stop();
    }
    else text="TCP connection failed";
  }
  if(cancelled)text+="\nCancelled";
  text.toCharArray(result.text,sizeof(result.text));
}
