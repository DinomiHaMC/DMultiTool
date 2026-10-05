#include "Firmware.h"
#include "../config.h"
void Firmware::begin()  {
  services.begin();
  apps.add(launcher);
  apps.add(wifi);
  apps.add(ble);
  apps.add(nfc);
  apps.add(ir);
  apps.add(files);
  apps.add(scripting);
  apps.add(tools);
  apps.add(settings);
  apps.add(games);
  apps.add(utils);
  ui.exit=[this]  {
    apps.launcher();
  };
  idle.begin();
  App* initial=apps.at(services.config.values.defaultApp);
  if(!initial||!apps.open(initial->name()))apps.launcher();
  ui.status=services.status();
  ui.toast(services.sd.mounted?"SD mounted":"SD card not mounted",services.sd.mounted?ToastType::Success:ToastType::Warning);
  ui.draw();
}
void Firmware::update()  {
  if(apps.foregroundBusy())idle.keepAwake();
  InputEvent event=services.input.poll(services.config.values);
  bool consumed=idle.update(services,event);
  services.update();
  if(services.bridge.notificationRevision!=notificationRevision) {
    notificationRevision=services.bridge.notificationRevision;
    if(services.config.values.notifyGlobal&&(!idle.asleep()||services.config.values.notifyWake)) {
      if(idle.asleep())idle.wake(services);
      notificationVisible=true;notificationAt=millis();ui.dirty=true;
      services.beep(2400,60);
    }
  }
  if(notificationVisible&&millis()-notificationAt>=5000) {
    notificationVisible=false;services.display.invalidate();ui.dirty=true;
  }
  if(!consumed&&event!=InputEvent::None)  {
    if(services.config.values.uiBeep)services.beep();
    if(services.config.values.hardwareDebug)LOG_DEBUG("INPUT","Event %d ADC %d",(int)event,services.input.raw);
    apps.handleInput(event);
  }
  if(services.irSent)ui.toast("IR sent",ToastType::Success);
  if(services.tagFound)ui.toast("NFC tag found",ToastType::Success);
  bool nowConnected=WiFi.status()==WL_CONNECTED;
  if(nowConnected!=connected)  {
    connected=nowConnected;
    ui.toast(connected?"WiFi connected":"WiFi disconnected",connected?ToastType::Success:ToastType::Warning);
    LOG_INFO("WIFI","%s",connected?"connected":"disconnected");
  }
  apps.update();
  ui.update();
  if(millis()-statusAt>1000)  {
    statusAt=millis();
    ui.status=services.status();
    ui.status.fps=frames;
    frames=0;
    ui.dirty=true;
  }
  if(ui.dirty&&!idle.asleep())  {
    apps.draw();
    if(notificationVisible)services.display.notification(services.bridge.notification,services.config.values.theme);
    frames++;
  }
  yield();
}
