#include "SettingsApp.h"
void SettingsApp::save()  {
  s.config.save();
  String title=ui.model().title;
  if(title=="Display")display(false);
  else if(title=="Sound")sound(false);
  else if(title=="Input")input(false);
  else if(title=="Interface")interface(false);
  else if(title=="Boot")boot(false);
  else if(title=="Developer")developer(false);
  ui.toast("Settings saved",ToastType::Success);
}
void SettingsApp::home()  {
  auto p=menu("Settings");
  item(p,"Display",[this]  {
    display();
  }
  );
  item(p,"Sound",[this]  {
    sound();
  }
  );
  item(p,"Input",[this]  {
    input();
  }
  );
  item(p,"WiFi",[this]  {
    ctx.apps->open("WiFi");
  }
  );
  item(p,"Bluetooth",[this]  {
    ctx.apps->open("Bluetooth");
  }
  );
  item(p,"NFC",[this]  {
    ctx.apps->open("NFC");
  }
  );
  item(p,"IR",[this]  {
    ctx.apps->open("Infrared");
  }
  );
  item(p,"Storage",[this]  {
    ui.message("Storage",String(s.sd.mounted?"SD mounted":"SD unavailable")+"\nNVS independent of SD\nNo automatic formatting\nLogs rotate at 1 MB");
  }
  );
  item(p,"Interface",[this]  {
    interface();
  }
  );
  item(p,"Boot",[this]  {
    boot();
  }
  );
  item(p,"Developer",[this]  {
    developer();
  }
  );
  item(p,"About",[this]  {
    ctx.apps->open("About");
  }
  );
  ui.page(std::move(p),false);
}
void SettingsApp::themes()  {
  auto p=menu("Theme");
  for(int i=0;i<4;i++)item(p,ThemeManager::get(i).name,[this,i]  {
    s.config.values.theme=i;s.display.invalidate();save();
  }
  );
  ui.page(std::move(p));
}
void SettingsApp::display(bool push)  {
  auto p=menu("Display");
  item(p,"Rotation: "+String(s.config.values.rotation),[this]  {
    s.config.values.rotation=(s.config.values.rotation+1)%4;s.display.rotation(s.config.values.rotation);save();
  }
  );
  item(p,"Screen timeout",[this]  {
    ui.numberInput("Timeout seconds (0 off)",s.config.values.timeout,0,600,[this](uint32_t value)  {
      s.config.values.timeout=value;save();
    }
    );
  },String(s.config.values.timeout)+" seconds");
  item(p,"Theme",[this]  {
    themes();
  },ThemeManager::get(s.config.values.theme).name);
  item(p,"Brightness fixed",[this]  {
    ui.message("Display","No backlight GPIO provided.\nScreen timeout disables pixels.");
  }
  );
  ui.page(std::move(p),push);
}
void SettingsApp::sound(bool push)  {
  auto p=menu("Sound");
  item(p,"Enabled",[this]  {
    s.config.values.sound=!s.config.values.sound;save();
  },s.config.values.sound?"On":"Off");
  item(p,"UI clicks",[this]  {
    s.config.values.uiBeep=!s.config.values.uiBeep;save();
  },s.config.values.uiBeep?"On":"Off");
  item(p,"Startup sound",[this]  {
    s.config.values.bootSound=!s.config.values.bootSound;save();
  },s.config.values.bootSound?"On":"Off");
  item(p,"Frequency",[this]  {
    ui.numberInput("Buzzer Hz",s.config.values.frequency,500,5000,[this](uint32_t value)  {
      s.config.values.frequency=value;save();s.beep();
    }
    );
  },String(s.config.values.frequency)+" Hz");
  ui.page(std::move(p),push);
}
void SettingsApp::input(bool push)  {
  auto p=menu("Input");
  item(p,"Repeat",[this]  {
    s.config.values.repeat=!s.config.values.repeat;save();
  },s.config.values.repeat?"On":"Off"
  );
  item(p,"Repeat delay",[this]  {
    ui.numberInput("Repeat delay ms",s.config.values.repeatDelay,200,1500,[this](uint32_t v)  {
      s.config.values.repeatDelay=v;save();
    }
    );
  },String(s.config.values.repeatDelay)+" ms"
  );
  item(p,"Repeat rate",[this]  {
    ui.numberInput("Repeat rate ms",s.config.values.repeatRate,60,500,[this](uint32_t v)  {
      s.config.values.repeatRate=v;save();
    }
    );
  },String(s.config.values.repeatRate)+" ms"
  );
  item(p,"Long press duration",[this]  {
    ui.numberInput("Long press ms",s.config.values.longPress,400,2000,[this](uint32_t v)  {
      s.config.values.longPress=v;save();
    }
    );
  },String(s.config.values.longPress)+" ms"
  );
  ui.page(std::move(p),push);
}
void SettingsApp::interface(bool push)  {
  auto p=menu("Interface");
  item(p,"Theme",[this]  {
    themes();
  }
  );
  item(p,"Animations",[this]  {
    s.config.values.animations=!s.config.values.animations;save();
  },s.config.values.animations?"On":"Off");
  item(p,"Menu wrap",[this]  {
    s.config.values.wrap=!s.config.values.wrap;save();
  },s.config.values.wrap?"On":"Off"
  );
  item(p,"Status bar",[this]  {
    s.config.values.statusBar=!s.config.values.statusBar;s.display.invalidate();save();
  },s.config.values.statusBar?"On":"Off"
  );
  ui.page(std::move(p),push);
}
void SettingsApp::boot(bool push)  {
  auto p=menu("Boot");
  item(p,"Splash",[this]  {
    s.config.values.splash=!s.config.values.splash;save();
  },s.config.values.splash?"On":"Off"
  );
  item(p,"Boot sound",[this]  {
    s.config.values.bootSound=!s.config.values.bootSound;save();
  },s.config.values.bootSound?"On":"Off"
  );
  item(p,"Default app",[this]  {
    auto list=menu("Default app");for(int i=0;i<ctx.apps->size();i++)  {
      auto* app=ctx.apps->at(i);item(list,app->name(),[this,i]  {
        s.config.values.defaultApp=i;save();
      }
      );
    }
    ui.page(std::move(list));
  }
  );
  ui.page(std::move(p),push);
}
void SettingsApp::developer(bool push)  {
  auto p=menu("Developer");
  item(p,"Serial logging",[this]  {
    s.config.values.serialLog=!s.config.values.serialLog;save();
  },s.config.values.serialLog?"On":"Off"
  );
  item(p,"Log level",[this]  {
    auto q=menu("Log level");const char* labels[]=  {
      "DEBUG","INFO","WARN","ERROR"
    };for(int i=0;i<4;i++)item(q,labels[i],[this,i]  {
      s.config.values.logLevel=i;save();
    }
    );ui.page(std::move(q));
  }
  );
  item(p,"Hardware debug",[this]  {
    s.config.values.hardwareDebug=!s.config.values.hardwareDebug;save();
  },s.config.values.hardwareDebug?"On":"Off"
  );
  item(p,"Debug overlay",[this]  {
    s.config.values.debugOverlay=!s.config.values.debugOverlay;save();
  },s.config.values.debugOverlay?"On":"Off"
  );
  ui.page(std::move(p),push);
}
