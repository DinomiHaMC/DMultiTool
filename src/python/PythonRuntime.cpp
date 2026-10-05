#include "PythonRuntime.h"
#include <freertos/task.h>
#include <esp_heap_caps.h>
void PythonRuntime::releaseQueues() {
  for(QueueHandle_t* queue:{&requests,&replies,&events,&logs})if(*queue) { vQueueDelete(*queue);*queue=nullptr; }
}
bool PythonRuntime::start(const String& source,const String& directory,int w,int h) {
  if(active||source.length()>4096||ESP.getFreeHeap()<125000||heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)<36000)return false;
  releaseQueues();
  code=(char*)malloc(source.length()+1);if(!code)return false;
  strcpy(code,source.c_str());
  requests=xQueueCreate(1,sizeof(Command));replies=xQueueCreate(1,sizeof(HPythonReply));
  events=xQueueCreate(16,sizeof(InputEvent));logs=xQueueCreate(4,192);
  if(!requests||!replies||!events||!logs) { free(code);code=nullptr;releaseQueues();return false; }
  workingDirectory=directory;width=w-16;height=h-112;
  view=PythonView();strcpy(view.status,"Running");generation++;started=millis();repaintAt=0;repaintPending=false;peak=0;
  cancelled=paused=done=false;active=alive=true;held=0;selected=-1;
  if(xTaskCreate(worker,"python",32768,this,1,nullptr)!=pdPASS) {
    active=alive=false;free(code);code=nullptr;releaseQueues();return false;
  }
  return true;
}
int PythonRuntime::interrupted(void* context) {
  auto& r=*(PythonRuntime*)context;
  while(r.paused&&!r.cancelled)vTaskDelay(pdMS_TO_TICKS(10));
  return r.cancelled||millis()-r.started>1800000;
}
void PythonRuntime::yieldWorker(void* context) {
  interrupted(context);vTaskDelay(1);
}
void PythonRuntime::logWorker(void* context,const char* text) {
  auto& r=*(PythonRuntime*)context;char line[192];snprintf(line,sizeof(line),"%s",text);xQueueSend(r.logs,line,0);
}
void PythonRuntime::worker(void* context) {
  auto& r=*(PythonRuntime*)context;
  HPythonHost host{context,exchange,interrupted,yieldWorker,logWorker};
  r.result=hp_python_run(r.code,&host,65536);r.peak=hp_python_peak();
  free(r.code);r.code=nullptr;r.alive=false;r.active=false;r.done=true;
  vTaskDelete(nullptr);
}
int PythonRuntime::exchange(void* context,const HPythonRequest* request,HPythonReply* response) {
  auto& r=*(PythonRuntime*)context;
  if(interrupted(context))return 0;
  switch(request->op) {
    case HP_SLEEP: {
      int ms=request->args[0];if(ms<0||ms>5000)return 0;
      uint32_t start=millis();
      do { if(interrupted(context))return 0;vTaskDelay(1); }while(millis()-start<(uint32_t)ms);
      response->value=1;return 1;
    }
    case HP_MILLIS:response->value=millis()&0x7FFFFFFF;return 1;
    case HP_ALIVE:response->value=r.alive;return 1;
    case HP_WIDTH:response->value=r.width;return 1;
    case HP_HEIGHT:response->value=r.height;return 1;
    case HP_SELECTED:response->value=r.selected;return 1;
    case HP_BUTTON: {
      InputEvent event=InputEvent::None;xQueueReceive(r.events,&event,0);
      const char* names[]={"","UP","DOWN","LEFT","RIGHT","OK","",""};
      snprintf(response->text,sizeof(response->text),"%s",names[(int)event]);return 1;
    }
    case HP_PRESSED: {
      const char* names[]={"","UP","DOWN","LEFT","RIGHT","OK",""};
      snprintf(response->text,sizeof(response->text),"%s",names[r.held.load()]);return 1;
    }
    default:break;
  }
  Command command{r.generation,*request};
  if(xQueueSend(r.requests,&command,pdMS_TO_TICKS(50))!=pdTRUE)return 0;
  uint32_t waited=0;
  while(!interrupted(context)) {
    uint32_t step=millis();
    if(xQueueReceive(r.replies,response,pdMS_TO_TICKS(20))==pdTRUE)return 1;
    waited+=millis()-step;
    if(waited>3000)return 0;
  }
  return 0;
}
void PythonRuntime::input(InputEvent event) {
  if(active&&!paused&&event>=InputEvent::Up&&event<=InputEvent::Ok)xQueueSend(events,&event,0);
}
void PythonRuntime::select(int delta) {
  if(!view.count)return;
  int cursor=view.selected;
  for(int i=0;i<view.count;i++) {
    cursor=(cursor+delta+view.count)%view.count;
    if(view.widgets[cursor].button) { view.selected=cursor;selected=cursor;return; }
  }
}
bool PythonRuntime::path(const char* name,String& output)const {
  String input=name;if(input.isEmpty()||input.indexOf('\\')>=0)return false;
  output=input.startsWith("/")?input:workingDirectory+"/"+input;
  if(output.length()>255||output.indexOf("/../")>=0||output.endsWith("/..")||output.indexOf("/./")>=0)return false;
  return true;
}
void PythonRuntime::command(ServiceManager& s,const HPythonRequest& r,HPythonReply& reply) {
  switch(r.op) {
    case HP_TITLE:snprintf(view.title,sizeof(view.title),"%s",r.text);break;
    case HP_LABEL:case HP_WIDGET: {
      if(view.count==view.widgets.size()) { reply.value=-1;return; }
      int id=view.count++;auto& widget=view.widgets[id];widget.button=r.op==HP_WIDGET;
      snprintf(widget.text,sizeof(widget.text),"%s",r.text);
      if(widget.button&&view.selected<0) { view.selected=id;selected=id; }
      reply.value=id;return;
    }
    case HP_SET_TEXT: {
      int id=r.args[0];if(id<0||id>=view.count)return;
      snprintf(view.widgets[id].text,sizeof(view.widgets[id].text),"%s",r.text);break;
    }
    case HP_RECT: {
      if(view.rectanglesCount==view.rectangles.size()) { reply.value=-1;return; }
      auto& box=view.rectangles[view.rectanglesCount];
      box.x1=constrain(r.args[0],0,width);box.y1=constrain(r.args[1],0,height);
      box.x2=constrain(r.args[2],box.x1,width);box.y2=constrain(r.args[3],box.y1,height);box.color=r.args[4];
      reply.value=view.rectanglesCount++;return;
    }
    case HP_CLEAR:view.rectanglesCount=0;view.rectangles.fill(PythonRectangle());break;
    case HP_DESTROY:alive=false;break;
    case HP_IR: {
      int a=r.args[0],c=r.args[1],n=r.args[2];
      if(a<0||a>65535||c<0||c>255||n<0||n>5)return;
      reply.value=s.ir.send({(uint16_t)a,(uint8_t)c,(uint8_t)n});return;
    }
    case HP_BEEP:if(r.args[0]>=100&&r.args[0]<=10000&&r.args[1]>=1&&r.args[1]<=1000)s.beep(r.args[0],r.args[1]);break;
    default: {
      String filename;if(!s.sd.mounted||!path(r.path,filename))return;
      if(r.op==HP_READ) {
        File f=SD.open(filename);if(!f||f.isDirectory()||f.size()>2048)return;
        size_t n=f.readBytes(reply.text,2048);reply.text[n]=0;reply.value=n;f.close();
      } else if(r.op==HP_WRITE||r.op==HP_APPEND) {
        File f=SD.open(filename,r.op==HP_WRITE?FILE_WRITE:FILE_APPEND);if(!f||f.isDirectory())return;
        reply.value=f.print(r.text)==strlen(r.text);f.close();
      } else if(r.op==HP_MKDIR)reply.value=!SD.exists(filename)&&SD.mkdir(filename);
      else if(r.op==HP_REMOVE)reply.value=s.sd.removeFile(filename);
      else if(r.op==HP_LIST) {
        File directory=SD.open(filename);if(!directory||!directory.isDirectory())return;
        size_t used=0;for(int i=0;i<64;i++) {
          File f=directory.openNextFile();if(!f)break;
          String name=f.name();size_t needed=name.length()+2;
          if(used+needed>=sizeof(reply.text)) { f.close();break; }
          used+=snprintf(reply.text+used,sizeof(reply.text)-used,"%s%s\n",name.c_str(),f.isDirectory()?"/":"");f.close();
        }
        directory.close();reply.value=used;
      }
      return;
    }
  }
  reply.value=1;
}
bool PythonRuntime::update(ServiceManager& s) {
  bool changed=false;held=(int)s.input.held();
  if(requests) {
    Command request;
    if(xQueuePeek(requests,&request,0)==pdTRUE&&(!paused||cancelled||!active)) {
      xQueueReceive(requests,&request,0);HPythonReply reply{};
      if(request.generation==generation&&active&&!cancelled) { command(s,request.request,reply);changed=true; }
      xQueueOverwrite(replies,&reply);
    }
  }
  if(logs) {
    char line[192];for(int i=0;i<4&&xQueueReceive(logs,line,0)==pdTRUE;i++) {
      snprintf(view.console,sizeof(view.console),"%s",line);LOG_INFO("PYTHON","%s",line);changed=true;
    }
  }
  if(done&&String(view.status)=="Running") {
    const char* names[]={"Finished","Cancelled","Memory limit","Python error"};
    snprintf(view.status,sizeof(view.status),"%s | peak %u KB",names[constrain(result,0,3)],(unsigned)(peak/1024));
    releaseQueues();changed=true;
  }
  repaintPending=repaintPending||changed;
  if(repaintPending&&millis()-repaintAt>=33) { repaintAt=millis();repaintPending=false;return true; }
  return false;
}
