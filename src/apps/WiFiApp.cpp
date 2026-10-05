#include "WiFiApp.h"
void WiFiApp::home()  {
  auto p=menu("WiFi");
  item(p,"Scanner",[this]  {
    scan();
  }
  );
  item(p,"Network Info",[this]  {
    networks();
  }
  );
  item(p,"Connect",[this]  {
    networks(true);
  }
  );
  item(p,"Saved Networks",[this]  {
    saved();
  }
  );
  item(p,"Current Connection",[this]  {
    current();
  }
  );
  item(p,"Host Scanner",[this]  {
    job(NetOperation::Hosts);
  },"Current subnet only; max 64");
  item(p,"Port Scanner",[this]  {
    job(NetOperation::Ports);
  },"TCP connect; max 128 ports");
  item(p,"Ping",[this]  {
    job(NetOperation::Ping);
  }
  );
  item(p,"DNS Lookup",[this]  {
    job(NetOperation::DNS);
  }
  );
  item(p,"TCP Client",[this]  {
    job(NetOperation::TCP);
  }
  );
  item(p,"TCP Listener",[this]  {
    job(NetOperation::Listen);
  }
  );
  item(p,"HTTP Client",[this]  {
    job(NetOperation::HTTP);
  }
  );
  item(p,"WiFi AP",[this]  {
    accessPoint();
  }
  );
  item(p,"Packet Monitor",[this]  {
    capture(false);
  }
  );
  item(p,"PCAP Capture",[this]  {
    capture(true);
  }
  );
  item(p,"MAC Info",[this]  {
    ui.message("MAC Info","STA: "+WiFi.macAddress()+"\nAP: "+WiFi.softAPmacAddress()+"\nBSSID: "+WiFi.BSSIDstr());
  }
  );
  item(p,"Settings",[this]  {
    s.config.values.wifi=!s.wifi.enabled;s.wifi.setEnabled(s.config.values.wifi);s.config.save();ui.toast(s.wifi.enabled?"WiFi enabled":"WiFi disabled");
  }
  );
  ui.page(std::move(p),false);
}
void WiFiApp::scan()  {
  if(s.capture.active)  {
    report(false,"","Stop capture first");
    return;
  }
  if(!s.wifi.scan())  {
    ui.message("WiFi","Enable WiFi / scan already running");
    return;
  }
  scanVersion=s.wifiRevision;
  scanWaiting=true;
  ui.progress("WiFi Scanner","Scanning channels...",[this]  {
    s.wifi.cancelScan();scanWaiting=false;
  }
  );
}
void WiFiApp::networks(bool connecting)  {
  if(s.wifi.scanning)  {
    ui.message("Networks","Scan in progress");
    return;
  }
  auto p=menu(connecting?"Connect network":"Networks");
  if(!s.wifi.count)item(p,"Scan first",[this]  {
    scan();
  }
  );
  for(int i=0;i<s.wifi.count;i++)  {
    auto& n=s.wifi.networks[i];
    int bars=constrain((n.rssi+100)/12,0,4);
    String detail=String(n.rssi)+"dBm CH"+n.channel+" "+s.wifi.auth(n.encryption)+" "+String("||||").substring(0,bars);
    item(p,n.ssid.isEmpty()?"<hidden>":n.ssid,[this,i,connecting]  {
      if(connecting)connect(i);else networkInfo(i);
    },detail,[this,i]  {
      connect(i);
    }
    );
  }
  ui.page(std::move(p));
}
void WiFiApp::networkInfo(int i)  {
  auto& n=s.wifi.networks[i];
  ui.rows("Network Info","SSID: "+n.ssid+"\nBSSID: "+n.bssid+"\nRSSI: "+n.rssi+" dBm\nChannel: "+n.channel+"\nEncryption: "+s.wifi.auth(n.encryption)+"\n"+(n.ssid.isEmpty()?"Hidden":"Visible")+" network");
  item(ui.model(),"Connect",[this,i]  {
    connect(i);
  }
  );
}
void WiFiApp::connect(int i)  {
  auto n=s.wifi.networks[i];
  auto begin=[this,n](String password)  {
    if(s.capture.active)  {
      report(false,"","Stop capture first");
      return;
    }
    connectSsid=n.ssid;
    WiFi.begin(n.ssid.c_str(),password.c_str());
    s.config.saveNetwork(n.ssid,password);
    connectedAt=millis();
    connectWaiting=true;
    ui.progress("Connecting",n.ssid,[this]  {
      WiFi.disconnect();connectWaiting=false;
    }
    );
  };
  if(n.ssid.isEmpty())  {
    ui.textInput("Hidden SSID","",[this,begin,n](String ssid)mutable  {
      auto hidden=n;hidden.ssid=ssid;ui.textInput("WiFi password","",[this,hidden](String password)  {
        WiFi.begin(hidden.ssid.c_str(),password.c_str());s.config.saveNetwork(hidden.ssid,password);connectSsid=hidden.ssid;connectedAt=millis();connectWaiting=true;ui.progress("Connecting",hidden.ssid,[this]  {
          WiFi.disconnect();connectWaiting=false;
        }
        );
      },64,true);
    },32);
  }
  else if(n.encryption==WIFI_AUTH_OPEN)begin("");
  else ui.textInput("WiFi password","",begin,64,true);
}
void WiFiApp::current()  {
  ui.rows("Connection","SSID: "+WiFi.SSID()+"\nIP: "+WiFi.localIP().toString()+"\nGateway: "+WiFi.gatewayIP().toString()+"\nSubnet: "+WiFi.subnetMask().toString()+"\nDNS: "+WiFi.dnsIP().toString()+"\nMAC: "+WiFi.macAddress()+"\nRSSI: "+WiFi.RSSI()+" dBm\nChannel: "+WiFi.channel());
  item(ui.model(),"Sync UTC clock",[this] {
    configTime(0,0,"pool.ntp.org");ui.toast("NTP requested (background)");
  }
  );
  item(ui.model(),"Disconnect",[this]  {
    WiFi.disconnect();ui.toast("Disconnected");
  }
  );
}
void WiFiApp::saved()  {
  auto p=menu("Saved Networks");
  for(int i=0;i<s.config.networkCount();i++)  {
    auto n=s.config.network(i);
    item(p,n.ssid,[this,n]  {
      if(!s.wifi.enabled)  {
        report(false,"","Enable WiFi first");return;
      }
      WiFi.begin(n.ssid.c_str(),n.password.c_str());connectSsid=n.ssid;connectedAt=millis();connectWaiting=true;ui.progress("Connecting",n.ssid,[this]  {
        WiFi.disconnect();connectWaiting=false;
      }
      );
    },"OK Connect | RIGHT Forget",[this,i,n]  {
      ui.confirm("Forget network?",n.ssid,[this,i]  {
        s.config.forgetNetwork(i);saved();
      }
      );
    }
    );
  }
  ui.page(std::move(p));
}
void WiFiApp::job(NetOperation operation)  {
  if(WiFi.status()!=WL_CONNECTED)  {
    ui.message("Diagnostics","Connect to your network first");
    return;
  }
  NetJob request;
  request.operation=operation;
  auto start=[this](NetJob job)  {
    startJob(job);
  };
  auto port=[this,start](NetJob job)  {
    ui.numberInput("Port",job.operation==NetOperation::Listen?5000:80,1,65535,[this,start,job](uint32_t port)mutable  {
      job.port=port;job.endPort=port;if(job.operation==NetOperation::Ports)  {
        ui.numberInput("Last port",port,port,min((uint32_t)65535,port+127),[start,job](uint32_t end)mutable  {
          job.endPort=end;start(job);
        }
        );
      }
      else ui.textInput(job.operation==NetOperation::Listen?"Reply text":"Send text","Hello",[start,job](String text)mutable  {
        text.toCharArray(job.text,sizeof(job.text));start(job);
      },256);
    }
    );
  };
  if(operation==NetOperation::Listen)  {
    port(request);
    return;
  }
  String initial=operation==NetOperation::HTTP?"http://":operation==NetOperation::DNS?"example.com":WiFi.gatewayIP().toString();
  if(operation==NetOperation::Hosts)  {
    IPAddress begin=WiFi.localIP();
    begin[3]=1;
    initial=begin.toString();
  }
  ui.textInput(operation==NetOperation::HTTP?"HTTP URL":"Host / IP",initial,[start,port,request](String host)mutable  {
    host.toCharArray(request.host,sizeof(request.host));if(request.operation==NetOperation::TCP||request.operation==NetOperation::Ports)port(request);else start(request);
  },180);
}
void WiFiApp::startJob(NetJob request)  {
  if(!s.net.start(request))  {
    ui.message("Diagnostics","Network disconnected / worker busy");
    return;
  }
  jobWaiting=true;
  jobVersion=s.net.revision;
  ui.progress("Network utility","Working...",[this]  {
    s.net.cancel();jobWaiting=false;
  }
  );
}
void WiFiApp::accessPoint()  {
  if(apActive)  {
    ui.confirm("Stop AP?",WiFi.softAPIP().toString(),[this]  {
      WiFi.softAPdisconnect(true);apActive=false;ui.toast("AP stopped");
    }
    );
    return;
  }
  ui.textInput("AP SSID",s.config.apSSID,[this](String ssid)  {
    ui.textInput("AP password",s.config.apPassword,[this,ssid](String password)  {
      if(!password.isEmpty()&&(password.length()<8||password.length()>63))  {
        report(false,"","Password: empty or 8..63 chars");return;
      }
      ui.numberInput("AP Channel",s.config.values.wifiChannel,1,13,[this,ssid,password](uint32_t channel)  {
        if(!s.wifi.enabled)  {
          s.wifi.setEnabled(true);s.config.values.wifi=true;s.config.save();
        }
        s.config.apSSID=ssid;s.config.apPassword=password;s.config.values.wifiChannel=channel;s.config.save(); apActive=WiFi.softAP(ssid.c_str(),password.isEmpty()?nullptr:password.c_str(),channel);report(apActive,"AP started");if(apActive)ui.message("Test AP",ssid+"\n"+WiFi.softAPIP().toString());
      }
      );
    },63,true);
  },32);
}
void WiFiApp::capture(bool save)  {
  if(!s.wifi.enabled||apActive||s.wifi.scanning)  {
    ui.message("Capture","Enable STA; stop AP and scanning");
    return;
  }
  if(save&&!s.sd.mounted)  {
    ui.message("Capture","SD card not mounted");
    return;
  }
  if(WiFi.status()!=WL_CONNECTED)  {
    ui.message("Capture","Connect to your own network first");
    return;
  }
  ui.confirm(save?"PCAP capture?":"Monitor packets?","Own BSSID management frames",[this,save]  {
    String file=s.uniquePath("/captures/wifi",".pcap");if(!s.capture.start(file,WiFi.BSSIDstr(),WiFi.channel(),save))  {
      report(false);return;
    }
    captureView=true;auto p=menu("Packet Monitor");item(p,"Packets: 0",  {
    }
    );item(p,"Stop",[this]  {
      s.capture.stop();captureView=false;ui.back();
    }
    );p.onBack=[this]  {
      s.capture.stop();captureView=false;ui.model().onBack=nullptr;ui.back();
    };p.hint="Passive / 30s / max 1MB";ui.page(std::move(p));
  }
  );
}
void WiFiApp::update()  {
  if(scanWaiting&&s.wifiRevision!=scanVersion&&!ui.isInput())  {
    scanWaiting=false;
    ui.model().onBack=nullptr;
    ui.back();
    networks();
  }
  if(jobWaiting&&s.net.revision!=jobVersion)  {
    jobWaiting=false;
    ui.model().onBack=nullptr;
    ui.back();
    ui.rows("Diagnostic result",s.net.output);
  }
  if(connectWaiting&&(WiFi.status()==WL_CONNECTED||millis()-connectedAt>15000))  {
    connectWaiting=false;
    ui.model().onBack=nullptr;
    ui.back();
    if(WiFi.status()==WL_CONNECTED)  {
      current();
      ui.toast("WiFi connected",ToastType::Success);
    }
    else  {
      WiFi.disconnect();
      ui.message("WiFi","Connection failed (15s)");
    }
  }
  if(captureView&&millis()-refreshAt>500)  {
    refreshAt=millis();
    ui.model().items[0].label="Packets: "+String(s.capture.packets.load());
    ui.model().items[0].detail="Dropped: "+String(s.capture.dropped.load());
    ui.dirty=true;
    if(!s.capture.active)  {
      captureView=false;
      ui.toast("Capture finished",ToastType::Success);
    }
  }
}
void WiFiApp::onClose()  {
  if(scanWaiting)s.wifi.cancelScan();
  if(connectWaiting)WiFi.disconnect();
  scanWaiting=jobWaiting=connectWaiting=captureView=false;
  s.net.cancel();
  if(s.capture.active)s.capture.stop();
}
