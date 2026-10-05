#pragma once
#include "PythonEngine.h"
#include "PythonView.h"
#include "../core/ServiceManager.h"
#include <atomic>
class PythonRuntime {
  struct Command { uint32_t generation;HPythonRequest request; };
  QueueHandle_t requests=nullptr,replies=nullptr,events=nullptr,logs=nullptr;
  char* code=nullptr;
  String workingDirectory;
  std::atomic<bool> active{false},cancelled{false},paused{false},done{false},alive{false};
  std::atomic<int> held{0},selected{-1};
  uint32_t generation=0,started=0,repaintAt=0;
  bool repaintPending=false;
  int result=0,width=224,height=208;
  size_t peak=0;
  static void worker(void*);
  static int exchange(void*,const HPythonRequest*,HPythonReply*);
  static int interrupted(void*);
  static void yieldWorker(void*);
  static void logWorker(void*,const char*);
  bool path(const char* name,String& output)const;
  void command(ServiceManager&,const HPythonRequest&,HPythonReply&);
  void releaseQueues();
public:
  PythonView view;
  bool start(const String& source,const String& directory,int displayWidth,int displayHeight);
  bool update(ServiceManager&);
  bool running()const { return active; }
  bool finished()const { return done; }
  void pause(bool value) { paused=value; }
  void stop() { cancelled=true;paused=false;alive=false; }
  void input(InputEvent event);
  void select(int delta);
};
