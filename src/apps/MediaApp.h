#pragma once
#include "MenuApp.h"
#include "../media/MediaDecoder.h"
class MediaApp:public MenuApp,private Media::Output {
  class FileSource:public Media::Source {
    File& file;
  public:
    uint32_t length=0;
    explicit FileSource(File& f):file(f) {}
    uint32_t size()const override { return length; }
    uint32_t position()const override { return file.position(); }
    bool seek(uint32_t offset)override { return offset<=length&&file.seek(offset); }
    size_t read(uint8_t* bytes,size_t count)override { int n=file.read(bytes,count);return n>0?size_t(n):0; }
  };
  File file;FileSource source{file};Media::Decoder decoder;Media::Movie movie;
  Media::Frame lastFrame;
  String directory="/",opened;
  uint32_t pageOffset=0,nextTick=0;
  uint8_t fps=10,originalRotation=0,currentRotation=0;
  bool active=false,video=false,bmp=false,paused=false,ended=false,loop=true,fromFiles=false,lastValid=false,redraw=false,needNext=false,aborted=false;
  std::array<InputEvent,8> pending{};uint8_t pendingCount=0;
  static bool supported(String name);
  void home()override;
  void browse(const String& path,uint32_t offset=0);
  void start(const String& path,bool viaFiles);
  void stop();
  void leave();
  void fail(const String& text);
  void adjacent(int direction);
  bool check()override;
  void begin(int x,int y,int width,int height)override { s.display.beginMedia(x,y,width,height); }
  bool block(int x,int y,uint16_t* pixels,int width,int height)override;
  void processPending();
public:
  using MenuApp::MenuApp;
  const char* name()const override { return "Media"; }
  Icon icon()const override { return Icon::File; }
  bool ownsNavigation()const override { return active; }
  void onOpenFile(const String& path)override { start(path,true); }
  void onClose()override { stop();s.display.invalidate(); }
  void update()override;
  void draw()override;
  void handleInput(InputEvent event)override;
};
