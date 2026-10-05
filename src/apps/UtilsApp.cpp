#include "UtilsApp.h"
#include "../services/AudioControl.h"
void UtilsApp::home() {
  auto p=menu("Utils");
  item(p,"AudioCtrl",[this]{startAudio();},"Bluetooth audio remote");
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
  if(ownsBLE)s.setBLEEnabled(false);
  audioActive=ownsBLE=false;
}
void UtilsApp::audioPage() {
  auto p=menu("AudioCtrl");
  p.hint="Hold OK: exit to Utils";
  // Four rows fit both portrait and landscape; arrows belong to the remote.
  item(p,phoneConnected?"Phone connected":"Waiting for phone",{},phoneConnected?lastCommand:"Pair: HP2000 HID");
  item(p,"UP / DOWN",{},"Volume + / -");
  item(p,"RIGHT / LEFT",{},"Next / previous track");
  item(p,"OK",{},"Play / pause");
  ui.page(std::move(p),false);
}
void UtilsApp::handleInput(InputEvent event) {
  if(!audioActive) { ui.handle(event);return; }
  auto result=AudioControl::handle(event,s.ble.connected(),[this](uint16_t command){return s.ble.consumer(command);});
  if(result==AudioControl::Result::Exit) { stopAudio();home();return; }
  if(result==AudioControl::Result::Disconnected) {
    ui.toast("Pair HP2000 HID on phone",ToastType::Warning);
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
  if(!audioActive)return;
  const bool connected=s.ble.connected();
  if(connected!=phoneConnected) {
    phoneConnected=connected;
    lastCommand="Ready";
    audioPage();
  }
}
