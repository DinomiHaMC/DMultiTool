#include "BLEService.h"
#include "BLEScanSession.h"
#include "../config.h"
#include <esp_heap_caps.h>
// NimBLE 2.5.1 includes the legacy bt-mem header, registering BOTH radio
// modes in core 3.3.12. This firmware only uses BLE. Release unused Classic
// memory during Arduino startup, before our heap guard and other services.
#if __has_include(<esp32-hal-alloc-ble-mem.h>)
#include <esp32-hal-alloc-ble-mem.h>
extern "C" bool btClassicInUse(void) { return false; }
#endif
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
static uint8_t reportMap[]=  {
  0x05,0x01,0x09,0x06,0xA1,0x01,0x85,0x01,0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,0x95,0x01,0x75,0x08,0x81,0x01,0x95,0x06,0x75,0x08,0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,0xC0, 0x05,0x01,0x09,0x02,0xA1,0x01,0x85,0x02,0x09,0x01,0xA1,0x00,0x05,0x09,0x19,0x01,0x29,0x03,0x15,0x00,0x25,0x01,0x95,0x03,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x05,0x81,0x01,0x05,0x01,0x09,0x30,0x09,0x31,0x09,0x38,0x15,0x81,0x25,0x7F,0x75,0x08,0x95,0x03,0x81,0x06,0xC0,0xC0, 0x05,0x0C,0x09,0x01,0xA1,0x01,0x85,0x03,0x15,0x00,0x26,0xFF,0x03,0x19,0x00,0x2A,0xFF,0x03,0x75,0x10,0x95,0x01,0x81,0x00,0xC0
};
bool BLEUtilityService::initializationMemoryLow() {
  return heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<75000;
}
bool BLEUtilityService::initialize()  {
  if(initialized)return true;
  const uint32_t free=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  const uint32_t largest=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  LOG_INFO("BLE","Starting: internal heap=%lu, largest=%lu",(unsigned long)free,(unsigned long)largest);
  if(free<75000) {
    error="BLE needs 75000 bytes RAM\nFree: "+String(free)+"\nDisable unused WiFi / tasks";
    LOG_WARN("BLE","%s",error.c_str());
    return false;
  }
  if(!NimBLEDevice::init("DMultiTool")) {
    error="BLE stack init failed\nFree RAM: "+String(free)+"\nSee Serial at 115200";
    LOG_ERROR("BLE","%s",error.c_str());
    return false;
  }
  scanner=NimBLEDevice::getScan();
  scanner->setScanCallbacks(&scanCallbacks);
  scanner->setActiveScan(true);
  scanner->setDuplicateFilter(0);
  scanner->setInterval(100);
  scanner->setWindow(60);
  scanner->setMaxResults(20);
  initialized=true;
  error="";
  LOG_INFO("BLE","Ready: heap=%lu",(unsigned long)ESP.getFreeHeap());
  return true;
}
void BLEUtilityService::BridgeCallbacks::onWrite(NimBLECharacteristic* characteristic,NimBLEConnInfo&) {
  const auto& value=characteristic->getValue();
  if(!owner.frames||value.size()<1||value.size()>sizeof(BLEFrame::bytes))return;
  BLEFrame frame;frame.size=value.size();memcpy(frame.bytes,value.data(),frame.size);
  xQueueSend(owner.frames,&frame,0);
}
bool BLEUtilityService::transmit(const uint8_t* bytes,size_t size) {
  if(!connected()||!bridgeTX||size>20)return false;
  return bridgeTX->notify(bytes,size);
}
bool BLEUtilityService::setEnabled(bool on)  {
  if(on&&!initialize())return false;
  enabled=on;
  if(!on) {
    stop();
    if(!scanning)shutdown();
  }
  return true;
}
void BLEUtilityService::shutdown() {
  if(!initialized||scanning)return;
  // Wait for scanWorker to finish copying results before invalidating SDK objects.
  if(!NimBLEDevice::deinit(false)) {
    LOG_WARN("BLE","Controller shutdown failed; retrying");
    return;
  }
  NimBLEDevice::deinit(true);
  delete hid;
  hid=nullptr;
  scanner=nullptr;
  server=nullptr;
  keyboard=mouse=media=nullptr;
  initialized=false;
  if(frames)vQueueDelete(frames);
  frames=nullptr;bridgeTX=nullptr;
  releasePending=false;
}
void BLEUtilityService::stop()  {
  scanCancelled=true;
  if(scanner&&scanning)scanner->stop();
  if(server)server->advertiseOnDisconnect(false);
  if(initialized)NimBLEDevice::getAdvertising()->stop();
  advertising=false;
  cancelTyping();
}
bool BLEUtilityService::scan()  {
  if(!enabled) { error="Enable Bluetooth first";return false; }
  if(scanning) { error="BLE scan already running";return false; }
  if(!initialize())return false;
  stop();
  error="";
  scanCancelled=false;scanEnded=false;
  scanning=true;
  scanDone=false;
  if(xTaskCreate(scanWorker,"ble-scan",4096,this,1,nullptr)!=pdPASS)  {
    scanning=false;
    error="Not enough RAM for BLE scan task";
    return false;
  }
  return true;
}
void BLEUtilityService::scanWorker(void* p)  {
  auto& s=*(BLEUtilityService*)p;
  // Start explicitly: blocking getResults(duration) also returns an empty
  // list on controller start failure, hiding that failure from the UI.
  auto outcome=BLEScanSession::run(*s.scanner,s.scanCancelled,[]{vTaskDelay(pdMS_TO_TICKS(20));},[&s]{return s.scanEnded.load();});
  if(outcome==BLEScanSession::Result::StartFailed) {
    s.error="BLE scan start failed\nConnection / controller busy\nRetry; see Serial at 115200";
    LOG_WARN("BLE","%s",s.error.c_str());
    s.scanning=false;s.scanDone=true;vTaskDelete(nullptr);return;
  }
  if(outcome==BLEScanSession::Result::Cancelled) {
    s.scanner->clearResults();s.count=0;
    s.scanning=false;s.scanDone=true;vTaskDelete(nullptr);return;
  }
  auto results=s.scanner->getResults();
  s.count=min((int)results.getCount(),20);
  for(int i=0;i<s.count;i++)  {
    const auto* d=results.getDevice(i);
    auto& info=s.devices[i];
    info.address=d->getAddress().toString().c_str();
    info.named=d->haveName()&&!d->getName().empty();
    info.name=info.named?String(d->getName().c_str()):"BLE "+info.address;
    info.rssi=d->getRSSI();
    info.services="";
    for(int k=0;k<min((int)d->getServiceUUIDCount(),4);k++)info.services+=String(d->getServiceUUID(k).toString().c_str())+"\n";
    info.manufacturer="";
    auto data=d->getManufacturerData();
    for(size_t k=0;k<min((size_t)32,data.length());k++)  {
      char b[4];
      snprintf(b,sizeof(b),"%02X ",(uint8_t)data[k]);
      info.manufacturer+=b;
    }
  }
  s.scanner->clearResults();
  s.scanning=false;
  s.scanDone=true;
  vTaskDelete(nullptr);
}
bool BLEUtilityService::connected()const  {
  return enabled&&server&&server->getConnectedCount()>0;
}
bool BLEUtilityService::advertise(const String& name,const String& manufacturer)  {
  if(!enabled||scanning||connected()||!initialize()||name.length()>18||manufacturer.length()>6)return false;
  auto* adv=NimBLEDevice::getAdvertising();
  adv->stop();
  NimBLEAdvertisementData data;
  data.setFlags(0x06);
  data.setName(name.c_str());
  data.setManufacturerData(manufacturer.c_str());
  if(!adv->setAdvertisementData(data))return false;
  adv->setMinInterval(0x320);
  adv->setMaxInterval(0x640);
  adv->start();
  advertising=true;
  return true;
}
bool BLEUtilityService::startHID(bool reconnect)  {
  if(!enabled||scanning||!initialize())return false;
  if(!hid)  {
    if(!frames)frames=xQueueCreate(4,sizeof(BLEFrame));
    if(!frames)return false;
    server=NimBLEDevice::createServer();
    auto* bridge=server->createService("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
    auto* rx=bridge->createCharacteristic("6e400002-b5a3-f393-e0a9-e50e24dcca9e",NIMBLE_PROPERTY::WRITE|NIMBLE_PROPERTY::WRITE_ENC,200);
    bridgeTX=bridge->createCharacteristic("6e400003-b5a3-f393-e0a9-e50e24dcca9e",NIMBLE_PROPERTY::NOTIFY,20);
    rx->setCallbacks(&bridgeCallbacks);
    hid=new NimBLEHIDDevice(server);
    keyboard=hid->getInputReport(1);
    mouse=hid->getInputReport(2);
    media=hid->getInputReport(3);
    hid->setManufacturer("DMultiTool");
    hid->setPnp(2,0xFFFF,0x2000,0x0200);
    hid->setHidInfo(0,1);
    hid->setReportMap(reportMap,sizeof(reportMap));
    NimBLEDevice::setSecurityAuth(true,false,true);
  }
  if(!server->start())return false;
  server->advertiseOnDisconnect(reconnect);
  if(connected())return true;
  auto* adv=NimBLEDevice::getAdvertising();
  adv->stop();
  NimBLEAdvertisementData data;
  data.setFlags(0x06);
  data.setName("DMultiTool HID");
  data.setCompleteServices(NimBLEUUID((uint16_t)0x1812));
  if(!adv->setAdvertisementData(data))return false;
  adv->setAppearance(HID_KEYBOARD);
  if(!adv->start())return false;
  advertising=true;
  return true;
}
bool BLEUtilityService::key(uint8_t usage,uint8_t modifier)  {
  if(!connected()||!keyboard)return false;
  uint8_t data[8]=  {
    modifier,0,usage,0,0,0,0,0
  };
  keyboard->setValue(data,8);
  keyboard->notify();
  releasePending=true;
  keyTime=millis();
  return true;
}
bool BLEUtilityService::consumer(uint16_t usage)  {
  if(!connected()||!media)return false;
  uint8_t data[2]=  {
    (uint8_t)usage,(uint8_t)(usage>>8)
  };
  media->setValue(data,2);
  if(!media->notify())return false;
  releasePending=true;
  keyTime=millis();
  return true;
}
bool BLEUtilityService::move(int8_t x,int8_t y,int8_t wheel,uint8_t buttons)  {
  if(!connected()||!mouse)return false;
  uint8_t data[4]=  {
    buttons,(uint8_t)x,(uint8_t)y,(uint8_t)wheel
  };
  mouse->setValue(data,4);
  mouse->notify();
  if(buttons)  {
    releasePending=true;
    keyTime=millis();
  }
  return true;
}
bool BLEUtilityService::typeText(const String& text)  {
  if(!connected()||text.length()>128)return false;
  for(size_t i=0;i<text.length();i++)if(text[i]<32||text[i]>126)return false;
  typing=text;
  typed=0;
  return true;
}
bool BLEUtilityService::update()  {
  bool changed=false;
  if(scanDone.exchange(false))  {
    revision++;
    changed=true;
    LOG_INFO("BLE","Scan: %d devices",count);
  }
  if(!enabled) {
    shutdown();
    return changed;
  }
  if(releasePending&&millis()-keyTime>=20)  {
    uint8_t zero[8]=  {
    };
    if(keyboard)  {
      keyboard->setValue(zero,8);
      keyboard->notify();
    }
    if(media)  {
      media->setValue(zero,2);
      media->notify();
    }
    if(mouse)  {
      mouse->setValue(zero,4);
      mouse->notify();
    }
    releasePending=false;
  }
  if(!releasePending&&typing.length()&&connected())  {
    char c=typing[typed++];
    uint8_t usage=0,mod=0;
    if(c>='a'&&c<='z')usage=c-'a'+4;
    else if(c>='A'&&c<='Z')  {
      usage=c-'A'+4;
      mod=2;
    }
    else if(c>='1'&&c<='9')usage=c-'1'+30;
    else if(c=='0')usage=39;
    else  {
      const char* plain=" -=[]\\;'/.,`",*shift=" _+{}|:\"?><~";
      const uint8_t codes[]=  {
        44,45,46,47,48,49,51,52,56,55,54,53
      };
      const char* p=strchr(plain,c);
      if(p)usage=codes[p-plain];
      else if((p=strchr(shift,c)))  {
        usage=codes[p-shift];
        mod=2;
      }
      else  {
        const char* symbols="!@#$%^&*()";
        p=strchr(symbols,c);
        if(p)  {
          usage=(p-symbols)==9?39:30+(p-symbols);
          mod=2;
        }
      }
    }
    if(usage)key(usage,mod);
    if(typed==typing.length())typing="";
  }
  return changed;
}
