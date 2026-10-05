#include "ToolsApp.h"
#include "../pins.h"
#include "../core/Version.h"
#include <esp_arduino_version.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
void ToolsApp::home()  {
  refresh=nullptr;
  auto p=menu("Tools");
  item(p,"System Info",[this]  {
    system();refresh=[this]  {
      system();
    };
  }
  );
  item(p,"Hardware Test",[this]  {
    hardware();refresh=[this]  {
      hardware();
    };
  }
  );
  item(p,"I2C Scanner",[this]  {
    s.nfc.scan(false);s.nfc.requestBusScan();i2c();refresh=[this]  {
      i2c();
    };
  }
  );
  item(p,"GPIO Viewer",[this]  {
    gpio();refresh=[this]  {
      gpio();
    };
  }
  );
  item(p,"ADC Monitor",[this]  {
    adc();refresh=[this]  {
      adc();
    };
  }
  );
  item(p,"SPI Info",[this]  {
    spi();
  }
  );
  item(p,"WiFi Diagnostics",[this]  {
    ctx.apps->open("WiFi");
  }
  );
  item(p,"Memory Monitor",[this]  {
    memory();refresh=[this]  {
      memory();
    };
  }
  );
  item(p,"Benchmark",[this]  {
    benchmark();
  }
  );
  item(p,"Serial Console",[this]  {
    serial();
  }
  );
  item(p,"Reboot",[this]  {
    ui.confirm("Reboot?","Restart ESP32",[]  {
      ESP.restart();
    }
    );
  }
  );
  ui.page(std::move(p),false);
}
void ToolsApp::system()  {
  String text="Chip: "+String(ESP.getChipModel())+" rev "+ESP.getChipRevision()+"\nCores: "+ESP.getChipCores()+"\nCPU: "+ESP.getCpuFreqMHz()+" MHz\nFlash: "+ESP.getFlashChipSize()/1024+" KB\nHeap: "+ESP.getHeapSize()+"\nFree: "+ESP.getFreeHeap()+"\nMinimum: "+ESP.getMinFreeHeap()+"\nMAC: "+WiFi.macAddress()+"\nUptime: "+millis()/1000+" s\nReset reason: "+(int)esp_reset_reason()+"\nCore: " ESP_ARDUINO_VERSION_STR "\nFirmware: " FW_VERSION "\nCommit: " FW_COMMIT;
  bool same=ui.model().title=="System Info";
  int selected=ui.model().selected;
  ui.rows("System Info",text,!same);
  if(same)ui.model().selected=constrain(selected,0,(int)ui.model().items.size()-1);
}
void ToolsApp::hardware()  {
  bool same=ui.model().title=="Hardware Test";
  int selected=ui.model().selected;
  ui.rows("Hardware Test",String("TFT: initialized\nKeyboard: ")+(s.input.seen?"OK event seen":"press key")+"\nSD: "+(s.sd.mounted?"OK":"FAIL")+"\nPN532: "+(s.nfc.available?"OK":"FAIL")+"\nIR TX: "+(s.ir.ready?"READY":"FAIL")+"\nBuzzer: OK init\nWiFi: "+(s.wifi.enabled?"READY":"OFF")+"\nBLE: "+(s.ble.enabled?"READY":"OFF"),!same);
  item(ui.model(),"Beep test",[this]  {
    s.beep(1800,150);
  }
  );
  if(same)ui.model().selected=constrain(selected,0,(int)ui.model().items.size()-1);
}
void ToolsApp::i2c()  {
  int selected=ui.model().selected;
  bool same=ui.model().title=="I2C Scanner";
  String text=s.nfc.busScanning?"Scanning...":"";
  for(size_t i=0;i<s.busResult.count;i++)text+="0x"+String(s.busResult.addresses[i],16)+(s.busResult.addresses[i]==0x24?" PN532":"")+"\n";
  if(text.isEmpty())text="No devices found";
  ui.rows("I2C Scanner",text,!same);
  item(ui.model(),"Rescan",[this]  {
    s.nfc.requestBusScan();
  }
  );
  if(same)ui.model().selected=constrain(selected,0,(int)ui.model().items.size()-1);
}
void ToolsApp::adc()  {
  bool same=ui.model().title=="ADC Monitor";
  static const char* names[]=  {
    "NONE","UP","DOWN","LEFT","RIGHT","OK","UNKNOWN"
  };
  ui.rows("ADC Monitor","GPIO36: "+String(s.input.raw)+"\nKey: "+names[(int)Keyboard::decode(s.input.raw)]+"\n12-bit ADC1\nStable debounce 35ms",!same);
}
void ToolsApp::memory()  {
  bool same=ui.model().title=="Memory Monitor";
  ui.rows("Memory Monitor","Free heap: "+String(ESP.getFreeHeap())+"\nMin heap: "+ESP.getMinFreeHeap()+"\nLargest block: "+heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)+"\nPSRAM: "+ESP.getPsramSize(),!same);
}
void ToolsApp::gpio()  {
  bool same=ui.model().title=="GPIO Viewer";
  String text="Read-only configured pins\n";
  for(uint8_t pin:  {
    Pins::TFT_CS,Pins::SD_CS,Pins::SDA,Pins::SCL,Pins::KEYBOARD,Pins::BUZZER,Pins::IR_TX
  }
  )text+="GPIO "+String(pin)+": "+digitalRead(pin)+"\n";
  ui.rows("GPIO Viewer",text,!same);
}
void ToolsApp::spi()  {
  ui.message("SPI Info","VSPI shared bus\nSCK18 MISO19 MOSI23\nTFT CS5 DC27 RST26\nSD CS13 10MHz\nMain task owns SPI\nNo inactive radio GPIO touched");
}
void ToolsApp::benchmark()  {
  volatile uint32_t value=0x12345678;
  uint32_t start=micros();
  for(uint32_t n=0;n<20000;n++)value=(value*1664525u)+1013904223u;
  uint32_t us=micros()-start;
  ui.message("Benchmark","20k integer operations\nTime: "+String(us)+" us\nChecksum: "+String(value,16));
}
void ToolsApp::serial()  {
  auto p=menu("Serial Console");
  item(p,"Send text",[this]  {
    ui.textInput("Serial send","",[this](String text)  {
      Serial.println(text);report(true,"Serial text sent");
    },128);
  }
  );
  item(p,"Received text",[this]  {
    ui.rows("Serial RX",s.serialText().isEmpty()?"No received bytes":s.serialText());
  }
  );
  item(p,"Refresh RX",[this]  {
    ui.rows("Serial RX",s.serialText());
  }
  );
  ui.page(std::move(p));
}
void ToolsApp::update()  {
  if(refresh&&!ui.isInput()&&millis()-refreshed>500)  {
    refreshed=millis();
    String title=ui.model().title;
    if(title=="System Info"||title=="Hardware Test"||title=="I2C Scanner"||title=="ADC Monitor"||title=="Memory Monitor"||title=="GPIO Viewer")refresh();
    else refresh=nullptr;
  }
}
