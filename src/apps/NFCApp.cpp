#include "NFCApp.h"
#include "FileManagerApp.h"
#include <time.h>
void NFCApp::home()  {
  auto p=menu("NFC");
  item(p,"Scan",[this]  {
    scan();
  }
  );
  item(p,"Tag Info",[this]  {
    info();
  }
  );
  item(p,"NDEF Reader",[this]  {
    ndefRead();
  }
  );
  item(p,"NDEF Writer",[this]  {
    auto q=menu("Write NDEF");item(q,"Text",[this]  {
      ndefWrite(false);
    }
    );item(q,"URL",[this]  {
      ndefWrite(true);
    }
    );ui.page(std::move(q));
  }
  );
  item(p,"Saved Tags",[this]  {
    ctx.apps->open("Files");static_cast<FileManagerApp*>(ctx.apps->find("Files"))->openDirectory("/nfc");
  }
  );
  item(p,"Tools",[this]  {
    ctx.apps->open("Tools");
  }
  );
  item(p,"Settings",[this]  {
    ui.message("NFC settings","PN532 I2C 0x24\nISO14443A UID\nNTAG + NDEF Classic 1K\nClassic uses standard NDEF keys");
  }
  );
  ui.page(std::move(p),false);
}
void NFCApp::scan()  {
  if(!s.nfc.available)  {
    ui.message("NFC","PN532 not detected");
    return;
  }
  waiting=true;
  s.nfc.scan(true);
  ui.progress("NFC Scan","Waiting for tag...",[this]  {
    s.nfc.scan(false);waiting=false;
  }
  );
}
void NFCApp::info()  {
  if(!s.nfc.last.length)  {
    ui.message("Tag Info","Scan a tag first");
    return;
  }
  bool classic=(s.nfc.last.sak&0x08)||(s.nfc.last.sak==0&&s.nfc.last.atqa==0x0004);
  ui.rows("NFC Tag",String("UID: ")+s.nfc.last.hex+"\nUID length: "+s.nfc.last.length+"\nTechnology: ISO14443A\nSAK: "+String(s.nfc.last.sak,HEX)+"\nATQA: "+String(s.nfc.last.atqa,HEX)+"\n"+(classic?"Classic 1K / compatible\n":"")+(s.nfc.last.length==7&&s.nfc.last.uid[0]==4?"UID manufacturer: NXP code":"Manufacturer: unknown"));
  auto& p=ui.model();
  item(p,"Save metadata",[this]  {
    save();
  }
  );
  item(p,"Scan again",[this]  {
    scan();
  }
  );
}
void NFCApp::save()  {
  String file=s.uniquePath("/nfc/tag",".json");
  time_t now=time(nullptr);
  String json="{\n  \"uid\": \""+String(s.nfc.last.hex)+"\",\n  \"uidLength\": "+String(s.nfc.last.length)+",\n  \"type\": \"ISO14443A\",\n  \"timestamp\": "+String(now>1700000000?(unsigned long)now:0)+"\n}\n";
  report(s.sd.writeNew(file,json),"File saved","NFC save failed / SD absent");
}
void NFCApp::ndefRead()  {
  if(!s.nfc.readNdef())  {
    ui.message("NDEF","PN532 unavailable or busy");
    return;
  }
  ndefVersion=s.nfc.ndefRevision;
  ndefWaiting=true;
  ui.progress("NDEF Reader","Present NTAG / Classic 1K (15s)",[this]  {
    s.nfc.scan(false);ndefWaiting=false;
  }
  );
}
void NFCApp::ndefWrite(bool uri)  {
  ui.textInput(uri?"URL":"NDEF text",uri?"https://":"",[this,uri](String text)  {
    if(uri&&!text.startsWith("https://")&&!text.startsWith("http://"))  {
      report(false,"","Use http:// or https://");return;
    }
    ui.confirm("Overwrite NDEF?","NTAG / NDEF Classic 1K",[this,text,uri]  {
      if(!s.nfc.writeNdef(text,uri))  {
        report(false,"","PN532 busy / unavailable");return;
      }
      ndefVersion=s.nfc.ndefRevision;ndefWaiting=true;ui.progress("NDEF Writer","Present tag (15s)",[this]  {
        s.nfc.scan(false);ndefWaiting=false;
      }
      );
    }
    );
  },100);
}
void NFCApp::update()  {
  if(waiting&&s.tagFound)  {
    waiting=false;
    ui.model().onBack=nullptr;
    ui.back();
    info();
    ui.toast("NFC tag found",ToastType::Success);
  }
  if(ndefWaiting&&s.nfc.ndefRevision!=ndefVersion)  {
    ndefWaiting=false;
    ui.model().onBack=nullptr;
    ui.back();
    ui.message("NDEF result",s.nfc.ndefText);
    ui.toast(s.nfc.ndefOk?"NDEF success":"NDEF failed",s.nfc.ndefOk?ToastType::Success:ToastType::Error);
  }
}
void NFCApp::onClose()  {
  waiting=ndefWaiting=false;
  s.nfc.scan(false);
}
