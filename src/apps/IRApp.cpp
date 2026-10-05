#include "IRApp.h"
void IRApp::home()  {
  rgbAddress=s.config.values.rgbAddress;
  rawFrequency=s.config.values.irCarrier;
  Preferences prefs;
  if(prefs.begin("hp-ir",true))  {
    for(int i=0;i<12;i++)  {
      String key="button"+String(i);
      String text=prefs.getString(key.c_str());
      assignedSet[i]=s.ir.parse(text,assigned[i]);
    }
    prefs.end();
  }
  auto p=menu("Infrared");
  item(p,"Remote",[this]  {
    remote();
  }
  );
  item(p,"RGB LED",[this]  {
    rgb();
  }
  );
  item(p,"NEC Sender",[this]  {
    sender();
  }
  );
  item(p,"Raw Sender",[this]  {
    raw();
  }
  );
  item(p,"Presets",[this]  {
    presets("/ir/presets");
  }
  );
  item(p,"Saved",[this]  {
    presets("/ir");
  }
  );
  item(p,"Settings",[this]  {
    ui.message("IR Settings","GPIO17 | NEC / Raw\nRaw 30..60 kHz\nMax 256 pulses / 200 ms\nIR receiver not installed");
  }
  );
  ui.page(std::move(p),false);
}
void IRApp::sender(bool push)  {
  auto p=menu("NEC Sender");
  item(p,"Address 0x"+String(signal.address,16),[this]  {
    ui.numberInput("Address HEX",signal.address,0,65535,[this](uint32_t v)  {
      signal.address=v;sender(false);
    },true);
  }
  );
  item(p,"Command 0x"+String(signal.command,16),[this]  {
    ui.numberInput("Command HEX",signal.command,0,255,[this](uint32_t v)  {
      signal.command=v;sender(false);
    },true);
  }
  );
  item(p,"Repeats "+String(signal.repeats),[this]  {
    ui.numberInput("Repeats",signal.repeats,0,5,[this](uint32_t v)  {
      signal.repeats=v;sender(false);
    }
    );
  }
  );
  item(p,"SEND",[this]  {
    report(s.ir.send(signal),"IR queued","IR busy or unavailable");
  }
  );
  item(p,"Save",[this]  {
    save();
  }
  );
  ui.page(std::move(p),push);
}
void IRApp::save()  {
  char b[64];
  snprintf(b,sizeof(b),"NEC %04X %02X %u\n",signal.address,signal.command,signal.repeats);
  report(s.sd.writeNew(s.uniquePath("/ir/nec",".ir"),b),"File saved","SD save failed");
}
void IRApp::presets(const String& directory)  {
  if(!s.sd.list(directory))  {
    ui.message("IR Files","SD card not mounted");
    return;
  }
  auto p=menu("IR Presets");
  item(p,"Test 0000 / 10",[this]  {
    signal=  {
      0,0x10,0
    };sender();
  }
  );
  for(size_t i=0;i<s.sd.count;i++)  {
    auto e=s.sd.entries[i];
    if(e.directory)continue;
    String full=directory+"/"+e.name;
    item(p,e.name,[this,full]  {
      load(full);
    }
    );
  }
  ui.page(std::move(p));
}
void IRApp::load(const String& file)  {
  File f=SD.open(file);
  if(!f||f.size()>512)  {
    report(false,"","Invalid / large preset");
    return;
  }
  String text=f.readString();
  f.close();
  IRSignal loaded;
  if(!s.ir.parse(text,loaded))  {
    report(false,"","Expected NEC addr cmd repeats");
    return;
  }
  if(assignIndex>=0)  {
    int i=assignIndex;
    assigned[i]=loaded;
    assignedSet[i]=true;
    Preferences prefs;
    if(prefs.begin("hp-ir",false))  {
      prefs.putString(("button"+String(i)).c_str(),text);
      prefs.end();
    }
    assignIndex=-1;
    remote();
    ui.toast("Button assigned",ToastType::Success);
  }
  else  {
    signal=loaded;
    sender();
  }
}
void IRApp::remote()  {
  assignIndex=-1;
  static const char* names[]=  {
    "Power","Vol+","Vol-","CH+","CH-","Up","Down","Left","Right","OK","Back","Mute"
  };
  auto p=menu("Remote");
  p.hint="OK Send  RIGHT Assign file";
  for(int i=0;i<12;i++)item(p,names[i],[this,i]  {
    report(assignedSet[i]&&s.ir.send(assigned[i]),"IR queued","Assign a saved NEC code first");
  },assignedSet[i]?"Assigned":"Not assigned",[this,i]  {
    assignIndex=i;presets("/ir");
  }
  );
  ui.page(std::move(p));
}
void IRApp::rgb()  {
  static const char* labels[]=  {
    "Brightness+","Brightness-","OFF","ON","Red","Green","Blue","White","Orange","Green 2","Blue 2","FLASH","Yellow","Cyan","Purple","STROBE","Amber","Teal","Pink","FADE","Lime","Aqua","Magenta","SMOOTH"
  };
  static const uint16_t colors[]=  {
    0,0,0,0,0xF800,0x07E0,0x001F,0xFFFF,0xFD20,0x87E0,0x041F,0,0xFFE0,0x07FF,0x8010,0,0xFCC0,0x0410,0xFB56,0,0xBFE0,0x87FF,0xF81F,0
  };
  auto p=menu("RGB LED 24-key");
  p.columns=4;
  p.hint="UP/DOWN rows RIGHT next OK send";
  for(int i=0;i<24;i++)  {
    item(p,labels[i],[this,i]  {
      IRSignal code  {
        rgbAddress,(uint8_t)i,0
      };report(s.ir.send(code),"IR queued","IR busy");
    },"Common NEC RGB profile");
    p.items.back().swatch=colors[i];
  }
  item(p,"Address",[this]  {
    ui.numberInput("RGB address HEX",rgbAddress,0,65535,[this](uint32_t value)  {
      rgbAddress=value;s.config.values.rgbAddress=value;s.config.save();
    },true);
  }
  );
  item(p,"Back",[this]  {
    ui.back();
  }
  );
  ui.page(std::move(p));
}
void IRApp::raw()  {
  auto p=menu("Raw Sender");
  item(p,"Load timings file",[this]  {
    ui.textInput("Raw file path","/ir/raw.ir",[this](String path)  {
      File file=SD.open(path);if(!file||file.size()>2000)  {
        report(false,"","Raw file absent / >2KB");return;
      }
      String text=file.readString();file.close();rawText(text);
    },128);
  }
  );
  item(p,"Enter timings",[this]  {
    ui.textInput("Timings us / CSV","9000,4500,560,560",[this](String text)  {
      rawText(text);
    },160);
  }
  );
  item(p,"Carrier kHz",[this]  {
    ui.numberInput("Carrier kHz",rawFrequency,30,60,[this](uint32_t value)  {
      rawFrequency=value;s.config.values.irCarrier=value;s.config.save();
    }
    );
  },String(rawFrequency)+" kHz");
  ui.page(std::move(p));
}
void IRApp::rawText(const String& source)  {
  String text=source;
  text.replace(","," ");
  const char* cur=text.c_str();
  char* end;
  std::vector<uint16_t> timings;
  bool valid=true;
  while(*cur&&timings.size()<256)  {
    while(isspace(*cur))cur++;
    if(!*cur)break;
    unsigned long value=strtoul(cur,&end,10);
    if(cur==end||!value||value>20000)  {
      valid=false;
      break;
    }
    timings.push_back(value);
    cur=end;
  }
  while(isspace(*cur))cur++;
  if(*cur||timings.empty())valid=false;
  if(!valid)  {
    report(false,"","Invalid raw timings");
    return;
  }
  ui.confirm("Send Raw?",String(rawFrequency)+" kHz / own target",[this,timings]  {
    report(s.ir.sendRaw(timings.data(),timings.size(),rawFrequency),"IR queued","IR busy / duration exceeds 200ms");
  }
  );
}
