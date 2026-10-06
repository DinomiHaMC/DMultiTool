#include "UtilsApp.h"
#include "../services/AudioControl.h"
void UtilsApp::home() {
  auto p=menu("Utils");
  item(p,"Morse",[this]{morse();},"Hold any arrow for tone | OK exit");
  item(p,"AudioCtrl",[this]{startAudio();},"Bluetooth audio remote");
  item(p,"Calculator",[this]{calculatorActive=true;s.display.invalidate();ui.dirty=true;},"Basic / scientific keyboard");
  item(p,"File transfer",[this]{transfer();},"microSD files over Bluetooth");
  item(p,"Notifications",[this]{notifications();},"Android notification bridge");
  ui.page(std::move(p),false);
}
void UtilsApp::startAudio() {
  if(s.ble.scanning) {
    ui.message("AudioCtrl","Stop BLE scan first");
    return;
  }
  ownsBLE=!s.ble.enabled;
  if(!s.setBLEEnabled(true)) {
    ownsBLE=false;
    ui.message("AudioCtrl",s.ble.lastError());
    return;
  }
  s.ble.cancelTyping();
  if(!s.ble.startHID(true)) {
    if(ownsBLE)s.setBLEEnabled(false);
    ownsBLE=false;
    ui.message("AudioCtrl","HID start failed\nTry again after closing BLE scan");
    return;
  }
  audioActive=true;
  phoneConnected=s.ble.connected();
  lastCommand="Ready";
  audioPage();
  if(s.wifiPaused())ui.toast("WiFi paused for BLE RAM",ToastType::Warning);
}
void UtilsApp::stopAudio() {
  if(!audioActive)return;
  s.ble.stop();
  if(ownsBLE&&!s.config.values.notifyReceive)s.setBLEEnabled(false);
  audioActive=ownsBLE=false;
}
void UtilsApp::audioPage() {
  auto p=menu("AudioCtrl");
  p.grid=false;p.columns=1;
  p.hint="Hold OK: exit to Utils";
  // Four rows fit both portrait and landscape; arrows belong to the remote.
  item(p,phoneConnected?"Phone connected":"Waiting for phone",{},phoneConnected?lastCommand:"Pair: DMultiTool HID");
  item(p,"UP / DOWN",{},"Volume + / -");
  item(p,"RIGHT / LEFT",{},"Next / previous track");
  item(p,"OK",{},"Play / pause");
  ui.page(std::move(p),false);
}
void UtilsApp::morse() {
  morseActive=true;morseDown=false;s.buzzer.beginKey();morsePage();
}
void UtilsApp::morsePage() {
  ui.rows("Morse",String(morseDown?"KEY DOWN":"Ready")+"\n700 Hz\nTap arrow: short signal\nHold arrow: long signal\nRelease: silence\nOK: exit",false);
  ui.model().hint="All arrows: tone | OK: exit";
}
void UtilsApp::handleInput(InputEvent event) {
  if(morseActive) {
    if(event==InputEvent::Ok||event==InputEvent::OkLong) { s.buzzer.endKey();morseActive=morseDown=false;home(); }
    return;
  }
  if(transferActive) {
    if(event==InputEvent::OkLong) { onClose();home(); }
    return;
  }
  if(calculatorActive) {
    if(event==InputEvent::OkLong) { calculatorActive=false;s.display.invalidate();home();return; }
    if(event==InputEvent::Left)calculator.move(-1,0);
    if(event==InputEvent::Right)calculator.move(1,0);
    if(event==InputEvent::Up)calculator.move(0,-1);
    if(event==InputEvent::Down)calculator.move(0,1);
    if(event==InputEvent::Ok)calculator.press();
    ui.dirty=true;return;
  }
  if(!audioActive) { ui.handle(event);return; }
  auto result=AudioControl::handle(event,s.ble.connected(),[this](uint16_t command){return s.ble.consumer(command);});
  if(result==AudioControl::Result::Exit) { stopAudio();home();return; }
  if(result==AudioControl::Result::Disconnected) {
    ui.toast("Pair DMultiTool HID on phone",ToastType::Warning);
    return;
  }
  if(result==AudioControl::Result::Failed) {
    ui.toast("Media command failed",ToastType::Error);
    return;
  }
  if(result==AudioControl::Result::Sent) {
    switch(event) {
      case InputEvent::Up:lastCommand="Volume +";break;
      case InputEvent::Down:lastCommand="Volume -";break;
      case InputEvent::Right:lastCommand="Next track";break;
      case InputEvent::Left:lastCommand="Previous track";break;
      default:lastCommand="Play / pause";break;
    }
    // Shows the command sent, without guessing the phone's playback state.
    ui.model().items[0].detail=lastCommand;
    ui.dirty=true;
  }
}
void UtilsApp::update() {
  if(morseActive) {
    Key key=s.input.held();bool down=key==Key::Up||key==Key::Down||key==Key::Left||key==Key::Right;
    s.buzzer.key(down);
    if(down!=morseDown) { morseDown=down;morsePage(); }
    return;
  }
  if(transferActive&&millis()-transferRefresh>500) { transferRefresh=millis();transferPage();return; }
  if(!audioActive)return;
  const bool connected=s.ble.connected();
  if(connected!=phoneConnected) {
    phoneConnected=connected;
    lastCommand="Ready";
    audioPage();
  }
}
void UtilsApp::transfer() {
  if(s.download.active||s.ble.scanning) { ui.message("File transfer","Finish download / BLE scan first");return; }
  transferOwnsBLE=!s.ble.enabled;
  if(!s.setBLEEnabled(true)||!s.ble.startBridge()) {
    if(transferOwnsBLE)s.setBLEEnabled(false);
    transferOwnsBLE=false;ui.message("File transfer","Bluetooth bridge failed\n"+s.ble.lastError());return;
  }
  transferActive=true;s.bridge.enable(true);transferPage();
}
void UtilsApp::transferPage() {
  ui.rows("File transfer",String(s.ble.connected()?"PC connected":"Pair DMultiTool HID")+"\n"+s.bridge.status+"\nPC: tools/mtble.py\nHold OK to stop",false);
  ui.model().hint="Hold OK: exit";
}
void UtilsApp::notifications(bool push) {
  auto p=menu("Notifications");
  item(p,"Receiver",[this] {
    bool next=!s.config.values.notifyReceive;
    if(next&&(!s.setBLEEnabled(true)||!s.ble.startBridge())) { ui.message("Notifications","Bluetooth start failed\n"+s.ble.lastError());return; }
    s.config.values.notifyReceive=next;
    if(next)s.config.values.ble=true;
    s.config.save();notifications(false);
  },s.config.values.notifyReceive?"On":"Off");
  item(p,"Global popups",[this] {
    s.config.values.notifyGlobal=!s.config.values.notifyGlobal;s.config.save();notifications(false);
  },s.config.values.notifyGlobal?"On":"Off");
  item(p,"Wake screen",[this] {
    s.config.values.notifyWake=!s.config.values.notifyWake;s.config.save();notifications(false);
  },s.config.values.notifyWake?"On":"Off");
  item(p,"Latest",[this]{ui.rows("Notification",s.bridge.notification.isEmpty()?"No notifications received":s.bridge.notification);});
  item(p,"Setup",[this]{ui.message("Android setup","Install companion/android app\nPair DMultiTool HID\nSelect device in companion\nGrant notification access\nEnable Receiver here");});
  ui.page(std::move(p),push);
}
void UtilsApp::onClose() {
  if(morseActive)s.buzzer.endKey();
  morseActive=morseDown=false;
  stopAudio();calculatorActive=false;
  if(transferActive) {
    s.bridge.enable(false);
    if(transferOwnsBLE&&!s.config.values.notifyReceive)s.setBLEEnabled(false);
    else if(!s.config.values.notifyReceive)s.ble.stop();
  }
  transferActive=transferOwnsBLE=false;
  s.display.invalidate();
}
