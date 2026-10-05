#include "PythonApp.h"
void PythonApp::home() { browse("/python"); }
void PythonApp::browse(const String& path,uint32_t page) {
  directory=path;offset=page;
  if(!s.sd.list(path,page)) { ui.message("Python","SD card not mounted / python directory missing");return; }
  ui.reset();auto p=menu(path);
  p.hint="OK Run / Open | Hold OK in app: exit";
  p.onBack=[this] {
    if(directory=="/python") { if(rootBack)rootBack();else ctx.apps->launcher(); }
    else { int slash=directory.lastIndexOf('/');browse(directory.substring(0,slash)); }
  };
  if(offset)item(p,"Previous files",[this]{browse(directory,offset-Config::MaxEntries);});
  for(size_t i=0;i<s.sd.count;i++) {
    auto entry=s.sd.entries[i];String full=directory+"/"+entry.name;
    p.items.push_back({entry.name,entry.directory?"Directory":String((unsigned long)entry.size)+" bytes",entry.directory?Icon::Folder:Icon::Python,[this,full,entry]{if(entry.directory)browse(full);else run(full);}});
  }
  if(s.sd.truncated)item(p,"Next files",[this]{browse(directory,offset+Config::MaxEntries);});
  item(p,"API / Help",[this]{ui.message("Python / PikaPython","Source <=4KB | VM heap <=64KB\nSmall TFT tkinter adapter:\nTk, Label, Button, Canvas\nbind / after / mainloop\ndevice.button()/pressed()\ndevice.ir_nec(addr,cmd,repeats)\nread_text/write_text/listdir\nHold OK -> confirm exit\nDesktop Tcl/Tk unavailable");});
  ui.page(std::move(p),false);
}
void PythonApp::run(const String& path) {
  File file=SD.open(path);
  if(!file||file.isDirectory()||file.size()>4096) { report(false,"","Python source missing / >4KB");return; }
  String source=file.readString();file.close();
  if(!runtime.start(source,directory,s.display.width(),s.display.height())) {
    ui.message("Python","Worker unavailable / insufficient RAM\nDisable unused BLE/WiFi and retry\nNeeds >=125KB free heap");return;
  }
  active=true;confirming=exiting=false;s.display.invalidate();ui.dirty=true;
}
void PythonApp::handleInput(InputEvent event) {
  if(!active) { ui.handle(event);return; }
  if(exiting)return;
  if(confirming) {
    if(event==InputEvent::BackLong)return;
    ui.handle(event);
    if(ui.model().title!="Exit Python?") {
      confirming=false;runtime.pause(false);s.display.invalidate();ui.dirty=true;
    }
    return;
  }
  if(event==InputEvent::OkLong) {
    confirming=true;runtime.pause(true);s.display.invalidate();
    ui.confirm("Exit Python?","Stop the program?",[this]{runtime.stop();exiting=true;confirming=false;ui.progress("Stopping Python","Releasing interpreter memory",{});});
    return;
  }
  if(event==InputEvent::BackLong)return;
  if(event==InputEvent::Up||event==InputEvent::Down) { runtime.select(event==InputEvent::Up?-1:1);ui.dirty=true; }
  runtime.input(event);
}
void PythonApp::update() {
  if(!active)return;
  if(runtime.update(s))ui.dirty=true;
  if(exiting&&runtime.finished()) {
    active=exiting=confirming=false;runtime.pause(false);s.display.invalidate();browse(directory,offset);
  }
}
void PythonApp::draw() {
  if(!active||confirming||exiting)ui.draw();
  else { s.display.renderPython(runtime.view,s.config.values.theme);ui.dirty=false; }
}
