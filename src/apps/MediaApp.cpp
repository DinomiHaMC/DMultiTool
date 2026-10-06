#include "MediaApp.h"
#include "FileManagerApp.h"
#include <esp_heap_caps.h>
namespace {
String parent(const String& path) { int slash=path.lastIndexOf('/');return slash<=0?"/":path.substring(0,slash); }
}
bool MediaApp::supported(String name) {
  name.toLowerCase();return name.endsWith(".jpg")||name.endsWith(".jpeg")||name.endsWith(".bmp")||name.endsWith(".mjpeg")||name.endsWith(".mjpg");
}
void MediaApp::home() {
  auto p=menu("Media");
  item(p,"Browse microSD",[this]{browse(SD.exists("/media")?"/media":"/");},"JPEG / BMP / MJPEG");
  item(p,"Video FPS",[this]{ui.numberInput("MJPEG target FPS",fps,2,20,[this](uint32_t value){fps=value;home();});},String(fps));
  item(p,"Loop video",[this]{loop=!loop;home();},loop?"On":"Off");
  item(p,"Help",[this]{ui.message("Media","Photos: JPEG / RGB BMP\nVideo: raw MJPEG (no audio)\nPC: tools/media_convert.py\nOK pause | Hold OK exit\nVideo LR seek, UD speed\nPhoto LR files, UD rotate");});
  ui.page(std::move(p),false);
}
void MediaApp::browse(const String& path,uint32_t offset) {
  directory=path;pageOffset=offset;
  if(!s.sd.list(path,offset)) { ui.message("Media","SD / directory unavailable");return; }
  ui.reset();auto p=menu("Media: "+path);
  p.onBack=[this] { if(directory=="/"||directory=="/media")home();else browse(parent(directory)); };
  if(offset)item(p,"Previous entries",[this]{browse(directory,pageOffset-Config::MaxEntries);});
  for(size_t i=0;i<s.sd.count;i++) {
    const auto& entry=s.sd.entries[i];String name=entry.name;
    String full=path=="/"?"/"+name:path+"/"+name;
    if(entry.directory)item(p,name,[this,full]{browse(full);},"Directory");
    else if(supported(name))item(p,name,[this,full]{start(full,false);},String((unsigned long)entry.size)+" bytes");
  }
  if(s.sd.truncated)item(p,"Next entries",[this]{browse(directory,pageOffset+Config::MaxEntries);});
  if(p.items.empty())item(p,"No supported media",[this]{ui.message("Formats","Use .jpg / .jpeg / .bmp\nVideo .mjpeg / .mjpg\nConvert MP4 / PNG on PC");});
  ui.page(std::move(p),false);
}
void MediaApp::stop() {
  active=false;file.close();decoder.close();pendingCount=0;aborted=false;
  if(currentRotation!=originalRotation)s.display.rotation(originalRotation);
  currentRotation=originalRotation;
}
void MediaApp::start(const String& path,bool viaFiles) {
  stop();opened=path;directory=parent(path);fromFiles=viaFiles;
  originalRotation=currentRotation=s.config.values.rotation;
  video=false;bmp=false;lastValid=ended=paused=aborted=false;redraw=needNext=true;pendingCount=0;movie.reset();
  String lower=path;lower.toLowerCase();video=lower.endsWith(".mjpeg")||lower.endsWith(".mjpg");bmp=lower.endsWith(".bmp");
  if(!supported(path)) { fail("Use JPEG / BMP / raw MJPEG");return; }
  file=SD.open(path);
  if(!file||file.isDirectory()||!file.size()||file.size()>1024ull*1024*1024) { fail("File unavailable / empty / >1 GiB");return; }
  source.length=file.size();
  if(!bmp&&heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<16000) { fail("JPEG needs free RAM\nDisable unused Bluetooth / tasks");return; }
  if(video) {
    File rate=SD.open(path+".fps");
    if(rate&&rate.size()<=4) { int value=rate.readString().toInt();if(value>=2&&value<=20)fps=value; }rate.close();
  }
  active=true;nextTick=millis();s.display.invalidate();ui.dirty=true;
}
void MediaApp::fail(const String& text) {
  stop();s.display.invalidate();ui.message("Media error",text);ui.model().onBack=[this]{leave();};
}
void MediaApp::leave() {
  String folder=directory;bool files=fromFiles;stop();s.display.invalidate();
  if(files) {
    ctx.apps->open("Files");auto* app=static_cast<FileManagerApp*>(ctx.apps->find("Files"));if(app)app->openDirectory(folder);
  } else browse(folder,pageOffset);
}
bool MediaApp::check() {
  if(aborted)return false;
  InputEvent event=s.input.poll(s.config.values);
  if(event==InputEvent::OkLong) { aborted=true;pendingCount=1;pending[0]=event;return false; }
  if(event!=InputEvent::None&&pendingCount<pending.size())pending[pendingCount++]=event;
  yield();return true;
}
bool MediaApp::block(int x,int y,uint16_t* pixels,int width,int height) {
  if(!check())return false;
  s.display.mediaBlock(x,y,pixels,width,height);return true;
}
void MediaApp::processPending() {
  auto events=pending;uint8_t count=pendingCount;pendingCount=0;aborted=false;
  for(uint8_t i=0;i<count&&active;i++)handleInput(events[i]);
}
void MediaApp::update() {
  if(!active)return;
  if(s.display.mediaNeedsRedraw())redraw=true;
  bool refresh=lastValid&&redraw;
  if(!refresh&&!needNext&&(!video||paused||int32_t(millis()-nextTick)<0))return;
  Media::Frame candidate=lastFrame;
  uint32_t started=millis(),index=0;Media::Result result=Media::Result::Ok;std::string message;
  if(!bmp&&!refresh) {
    if(video)result=movie.next(source,candidate,index,message,this);
    else result=Media::findJPEG(source,0,candidate,message,this);
    if(result==Media::Result::End) {
      if(!lastValid)fail("No JPEG frames in file");
      else if(loop&&video) { movie.reset();lastValid=false;needNext=true;nextTick=millis(); }
      else { paused=ended=true;needNext=false;ui.dirty=true; }
      processPending();return;
    }
    if(result==Media::Result::Ok&&video&&index<movie.target) { processPending();return; }
  }
  if(result==Media::Result::Ok)result=bmp?decoder.bmp(source,*this,s.display.width(),s.display.height()-52):decoder.jpeg(source,candidate,*this,s.display.width(),s.display.height()-52);
  if(result==Media::Result::Error) { String reason=message.empty()?decoder.error.c_str():message.c_str();fail(reason);return; }
  if(result==Media::Result::Ok) {
    lastFrame=candidate;lastValid=true;redraw=needNext=false;
    if(video&&!refresh)movie.presented=index;
    uint32_t interval=1000/fps;nextTick=started+interval;if(int32_t(millis()-nextTick)>0)nextTick=millis();
    ui.dirty=true;
  }
  processPending();
}
void MediaApp::adjacent(int direction) {
  File folder=SD.open(directory);if(!folder||!folder.isDirectory())return;
  String first,last,previous,choice;bool found=false;
  for(;;) {
    File entry=folder.openNextFile();if(!entry)break;
    bool isFile=!entry.isDirectory();String name=entry.name();entry.close();name=name.substring(name.lastIndexOf('/')+1);
    if(!isFile||!supported(name))continue;
    String path=directory=="/"?"/"+name:directory+"/"+name;
    if(first.isEmpty())first=path;
    last=path;
    if(path==opened) { found=true;if(direction<0&&!previous.isEmpty()) { choice=previous;break; } }
    else if(found&&direction>0) { choice=path;break; }
    previous=path;
  }
  folder.close();if(choice.isEmpty())choice=direction>0?first:last;
  if(!choice.isEmpty())start(choice,fromFiles);
}
void MediaApp::handleInput(InputEvent event) {
  if(!active) { ui.handle(event);return; }
  if(event==InputEvent::OkLong) { leave();return; }
  if(event==InputEvent::BackLong)return;
  if(video) {
    if(event==InputEvent::Ok) {
      if(ended) { movie.reset();lastValid=false;ended=false;needNext=true;paused=false; }else paused=!paused;
      nextTick=millis();ui.dirty=true;
    }
    if(event==InputEvent::Up||event==InputEvent::Down) { fps=constrain(int(fps)+(event==InputEvent::Up?1:-1),2,20);ui.dirty=true; }
    if(event==InputEvent::Left||event==InputEvent::Right) { movie.seekFrames((event==InputEvent::Left?-1:1)*(paused?1:fps));ended=false;needNext=true;redraw=false;nextTick=millis(); }
  } else {
    if(event==InputEvent::Left||event==InputEvent::Right)adjacent(event==InputEvent::Left?-1:1);
    if(event==InputEvent::Up||event==InputEvent::Down) { currentRotation=(currentRotation+(event==InputEvent::Up?1:3))%4;s.display.rotation(currentRotation);redraw=true;ui.dirty=true; }
  }
}
void MediaApp::draw() {
  if(!active) { ui.draw();return; }
  String title=opened.substring(opened.lastIndexOf('/')+1);
  String hint=video?String(ended?"END":paused?"PAUSE":"PLAY")+" "+String(fps)+"fps #"+String((unsigned long)(movie.presented+1))+" | Hold OK exit":"LR files | UD rotate | Hold OK exit";
  s.display.mediaStatus(title,hint);ui.dirty=false;
}
