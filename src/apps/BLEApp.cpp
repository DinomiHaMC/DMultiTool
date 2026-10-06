#include "BLEApp.h"
bool BLEApp::ensure()  {
  if(s.ble.enabled)return true;
  if(!s.setBLEEnabled(true))  {
    ui.message("Bluetooth",s.ble.lastError());
    return false;
  }
  s.config.values.ble=true;
  s.config.save();
  if(s.wifiPaused())ui.toast("WiFi paused for BLE RAM",ToastType::Warning);
  return true;
}
void BLEApp::home()  {
  auto p=menu("Bluetooth / BLE");
  item(p,"Scanner",[this]  {
    scan();
  }
  );
  item(p,"Devices",[this]  {
    devices();
  }
  );
  item(p,"Device Info",[this]  {
    devices();
  }
  );
  item(p,"Advertiser",[this]  {
    advertiser();
  }
  );
  item(p,"BLE Keyboard",[this]  {
    keyboard();
  }
  );
  item(p,"BLE Mouse",[this]  {
    mouse();
  }
  );
  item(p,"Settings",[this]  {
    bool next=!s.ble.enabled;
    if(!s.setBLEEnabled(next)) {
      ui.message("Bluetooth",s.ble.lastError());
      return;
    }
    report(true,next?(s.wifiPaused()?"BLE ON; WiFi paused":"BLE enabled"):"BLE disabled");
    s.config.values.ble=s.ble.enabled;s.config.save();
  }
  );
  ui.page(std::move(p),false);
}
void BLEApp::scan()  {
  if(!ensure())return;
  if(!s.ble.scan())  {
    ui.message("BLE Scanner",s.ble.lastError());
    return;
  }
  scanVersion=s.ble.revision;
  scanWaiting=true;
  ui.progress("BLE Scanner","Active BLE scan 6 seconds",[this]  {
    s.ble.stop();scanWaiting=false;
  }
  );
}
void BLEApp::devices()  {
  if(s.ble.scanning)  {
    ui.message("BLE","Scan in progress");
    return;
  }
  auto p=menu("BLE Devices");
  for(int i=0;i<s.ble.count;i++)item(p,s.ble.devices[i].name,[this,i]  {
    info(i);
  },s.ble.devices[i].address+" "+s.ble.devices[i].rssi+"dBm");
  if(!s.ble.count)p.hint="BLE only; enable nearby advertising";
  if(!s.ble.count)item(p,"Run Scanner",[this]  {
    scan();
  }
  );
  ui.page(std::move(p));
}
void BLEApp::info(int i)  {
  auto& d=s.ble.devices[i];
  ui.rows("BLE Device",d.name+(d.named?"":"\nNo advertised name")+"\nAddress: "+d.address+"\nRSSI: "+d.rssi+" dBm\nServices:\n"+d.services+"Manufacturer hex:\n"+d.manufacturer);
}
void BLEApp::advertiser()  {
  if(!ensure())return;
  if(s.ble.advertising)  {
    ui.confirm("Stop advertising?","DMultiTool own advertisement",[this]  {
      s.ble.stop();ui.toast("Advertisement stopped");
    }
    );
    return;
  }
  ui.textInput("Advertised name","DMultiTool",[this](String name)  {
    ui.textInput("Manufacturer bytes","DMT",[this,name](String data)  {
      report(s.ble.advertise(name,data),"Advertising (0.5..1s)","Scan/connection busy or invalid data");
    },6);
  },18);
}
void BLEApp::keyboard()  {
  if(!ensure())return;
  if(!s.ble.startHID())  {
    report(false,"","HID unavailable / scanning");
    return;
  }
  auto p=menu("BLE Keyboard");
  p.hint="Pair DMultiTool HID on own host";
  item(p,"Type text",[this]  {
    ui.textInput("Type ASCII","",[this](String text)  {
      for(size_t i=0;i<text.length();i++)if((uint8_t)text[i]>126) {
        ui.message("BLE Keyboard","HID typing requires ASCII / US host layout");return;
      }
      ui.confirm("Type on host?","Own paired device",[this,text]  {
        report(s.ble.typeText(text),"Typing","Pair/connect host first");
      }
      );
    },128);
  }
  );
  item(p,"Enter",[this]  {
    report(s.ble.key(0x28),"Enter sent","Host not connected");
  }
  );
  static const char* names[]=  {
    "Up","Down","Left","Right"
  };
  static const uint8_t codes[]=  {
    0x52,0x51,0x50,0x4F
  };
  for(int i=0;i<4;i++)item(p,names[i],[this,i]  {
    report(s.ble.key(codes[i]),"Key sent","Host not connected");
  }
  );
  item(p,"Play / Pause",[this]  {
    report(s.ble.consumer(0xCD),"Media sent","Host not connected");
  }
  );
  item(p,"Volume+",[this]  {
    report(s.ble.consumer(0xE9),"Media sent");
  }
  );
  item(p,"Volume-",[this]  {
    report(s.ble.consumer(0xEA),"Media sent");
  }
  );
  item(p,"Stop typing",[this]  {
    s.ble.cancelTyping();
  }
  );
  ui.page(std::move(p));
}
void BLEApp::mouse()  {
  if(!ensure())return;
  if(!s.ble.startHID())  {
    report(false);
    return;
  }
  auto p=menu("BLE Mouse");
  item(p,"Up",[this]  {
    report(s.ble.move(0,-10,0),"Moved");
  }
  );
  item(p,"Down",[this]  {
    report(s.ble.move(0,10,0),"Moved");
  }
  );
  item(p,"Left",[this]  {
    report(s.ble.move(-10,0,0),"Moved");
  }
  );
  item(p,"Right",[this]  {
    report(s.ble.move(10,0,0),"Moved");
  }
  );
  item(p,"Click",[this]  {
    report(s.ble.move(0,0,0,1),"Clicked");
  }
  );
  item(p,"Scroll+",[this]  {
    report(s.ble.move(0,0,1),"Scrolled");
  }
  );
  item(p,"Scroll-",[this]  {
    report(s.ble.move(0,0,-1),"Scrolled");
  }
  );
  ui.page(std::move(p));
}
void BLEApp::update()  {
  if(scanWaiting&&s.ble.revision!=scanVersion)  {
    scanWaiting=false;
    ui.model().onBack=nullptr;
    ui.back();
    if(!s.ble.lastError().isEmpty())ui.message("BLE Scanner",s.ble.lastError());
    else devices();
  }
}
