#include "ScriptApp.h"
void ScriptApp::home()  {
  auto p=menu("Scripts");
  if(!s.sd.list("/scripts"))  {
    ui.message("Scripts","SD card not mounted");
    return;
  }
  for(size_t i=0;i<s.sd.count;i++)  {
    auto e=s.sd.entries[i];
    String name=e.name;
    if(e.directory||!name.endsWith(".script"))continue;
    String path="/scripts/"+name;
    item(p,name,[this,path]  {
      ui.confirm("Run script?","Own device actions",[this,path]  {
        start(path);
      }
      );
    }
    );
  }
  item(p,"Format",[this]  {
    ui.message("Script format","PRINT \"text\"\nBEEP Hz ms\nDELAY ms\nIR_NEC address command repeats\nWIFI_SCAN\nNFC_SCAN\nOPEN_APP \"app name\"\nMax 256 lines / 60 seconds");
  }
  );
  ui.page(std::move(p),false);
}
void ScriptApp::start(const String& path)  {
  file=SD.open(path);
  if(!file||file.size()>16384)  {
    report(false,"","Script absent / >16KB");
    return;
  }
  filename=path;
  output="";
  line=0;
  started=millis();
  deadline=started;
  running=true;
  waitingWiFi=waitingNFC=false;
  show();
}
void ScriptApp::show()  {
  auto p=menu("Script Runner");
  item(p,"Line "+String(line),  {
  },running?"Running":"Stopped");
  item(p,output.isEmpty()?filename:output,  {
  }
  );
  item(p,"Cancel",[this]  {
    stop("Cancelled");
  }
  );
  p.onBack=[this]  {
    stop("Cancelled");
    home();
  };
  ui.page(std::move(p),false);
}
void ScriptApp::stop(const String& text)  {
  running=false;
  waitingWiFi=waitingNFC=false;
  s.nfc.scan(false);
  file.close();
  output=text;
  show();
}
void ScriptApp::onClose()  {
  running=false;
  waitingWiFi=waitingNFC=false;
  file.close();
  s.nfc.scan(false);
}
static bool number(const std::string& value,uint32_t& n)  {
  char* end=nullptr;
  unsigned long v=strtoul(value.c_str(),&end,0);
  if(value.empty()||*end||value[0]=='-')return false;
  n=v;
  return true;
}
void ScriptApp::execute(const String& source)  {
  std::vector<std::string> t;
  if(!Script::tokenize(source.c_str(),t))  {
    stop("Syntax error");
    return;
  }
  if(t.empty())return;
  auto fail=[this]  {
    stop("Bad command/args at line "+String(line));
  };
  uint32_t a=0,b=0,c=0;
  if(t[0]=="PRINT"&&t.size()==2)  {
    output=t[1].c_str();
    LOG_INFO("SCRIPT","%s",output.c_str());
  }
  else if(t[0]=="BEEP"&&t.size()==3&&number(t[1],a)&&number(t[2],b)&&a>=100&&a<=10000&&b>=1&&b<=1000)  {
    s.beep(a,b);
  }
  else if(t[0]=="DELAY"&&t.size()==2&&number(t[1],a)&&a<=5000)  {
    deadline=millis()+a;
  }
  else if(t[0]=="IR_NEC"&&t.size()==4&&number(t[1],a)&&number(t[2],b)&&number(t[3],c)&&a<=65535&&b<=255&&c<=5)  {
    IRSignal signal  {
      (uint16_t)a,(uint8_t)b,(uint8_t)c
    };
    if(!s.ir.send(signal))fail();
  }
  else if(t[0]=="WIFI_SCAN"&&t.size()==1)  {
    if(!s.wifi.scan())  {
      fail();
      return;
    }
    waitingWiFi=true;
    waitStarted=millis();
  }
  else if(t[0]=="NFC_SCAN"&&t.size()==1)  {
    if(!s.nfc.available)  {
      fail();
      return;
    }
    s.nfc.scan(true);
    waitingNFC=true;
    waitStarted=millis();
  }
  else if(t[0]=="OPEN_APP"&&t.size()==2)  {
    String name=t[1].c_str();
    if(name=="Scripts"||!ctx.apps->find(name))  {
      fail();
      return;
    }
    onClose();
    ctx.apps->open(name);
  }
  else fail();
}
void ScriptApp::update()  {
  if(!running)return;
  if(millis()-started>60000||line>=256)  {
    stop("Execution limit reached");
    return;
  }
  if(waitingWiFi)  {
    if(s.wifi.scanning&&millis()-waitStarted<15000)return;
    waitingWiFi=false;
    output="WiFi networks: "+String(s.wifi.count);
  }
  if(waitingNFC)  {
    if(!s.tagFound&&millis()-waitStarted<15000)return;
    waitingNFC=false;
    s.nfc.scan(false);
    output=s.tagFound?String(s.nfc.last.hex):"NFC timeout";
  }
  if(s.ir.busy||(int32_t)(millis()-deadline)<0)return;
  if(!file.available())  {
    stop("Finished");
    return;
  }
  String source;
  source.reserve(164);
  while(file.available())  {
    char c=file.read();
    if(c=='\n')break;
    if(c!='\r')source+=c;
    if(source.length()>160)  {
      stop("Line >160 bytes");
      return;
    }
  }
  line++;
  execute(source);
  if(running)show();
}
