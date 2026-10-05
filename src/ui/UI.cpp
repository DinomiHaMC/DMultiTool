#include "UI.h"
#include "GridNavigation.h"
void UI::reset()  {
  history.clear();
  editing=false;
  inputDone=nullptr;
  current=MenuPage();
  dirty=true;
}
void UI::page(MenuPage p,bool push)  {
  if(!push&&current.title==p.title)p.selected=current.selected;
  if(push&&history.size()>=16)  {
    toast("Navigation limit; go back",ToastType::Warning);
    return;
  }
  if(push&&current.items.size()&&history.size()<16)history.push_back(current);
  current=std::move(p);
  if(current.items.empty())current.items.push_back(  {
    "(empty)","",Icon::Info
  }
  );
  current.selected=constrain(current.selected,0,(int)current.items.size()-1);
  dirty=true;
}
void UI::back()  {
  if(editing)  {
    editing=false;
    inputDone=nullptr;
    display.invalidate();
    dirty=true;
    return;
  }
  if(current.onBack)  {
    auto callback=current.onBack;
    callback();
    return;
  }
  if(history.empty())  {
    if(exit)exit();
  }
  else  {
    current=std::move(history.back());
    history.pop_back();
    dirty=true;
  }
}
void UI::handle(InputEvent e)  {
  if(editing) {
    if(e==InputEvent::Up)keyboard.move(KeyboardModel::Direction::Up);
    else if(e==InputEvent::Down)keyboard.move(KeyboardModel::Direction::Down);
    else if(e==InputEvent::Left)keyboard.move(KeyboardModel::Direction::Left);
    else if(e==InputEvent::Right)keyboard.move(KeyboardModel::Direction::Right);
    else if(e==InputEvent::OkLong) { back();return; }
    else if(e==InputEvent::Ok && keyboard.press()) {
      String value=keyboard.value.c_str();
      auto callback=inputDone;
      inputDone=nullptr;
      editing=false;
      display.invalidate();
      dirty=true;
      if(callback)callback(value);
      return;
    }
    dirty=true;
    return;
  }
  if(e==InputEvent::OkLong)  {
    back();
    return;
  }
  int n=current.items.size();
  if(!n)return;
  if(current.grid&&(e==InputEvent::Up||e==InputEvent::Down||e==InputEvent::Left||e==InputEvent::Right)) {
    current.selected=GridNavigation::move(current.selected,n,3,e==InputEvent::Left?-1:e==InputEvent::Right?1:0,e==InputEvent::Up?-1:e==InputEvent::Down?1:0,settings.wrap);
    dirty=true;
    return;
  }
  if(e==InputEvent::Up||e==InputEvent::Down)  {
    int next=current.selected+(e==InputEvent::Up?-(int)current.columns:(int)current.columns);
    current.selected=settings.wrap?(next+n)%n:constrain(next,0,n-1);
    dirty=true;
    return;
  }
  if(e==InputEvent::Right&&current.columns>1)  {
    current.selected=(current.selected+1)%n;
    dirty=true;
    return;
  }
  auto item=current.items[current.selected];
  if(!item.enabled)  {
    toast("Not installed",ToastType::Warning);
    return;
  }
  Action callback=e==InputEvent::Right?item.context:e==InputEvent::Ok?item.action:Action();
  if(callback)callback();
}
void UI::draw()  {
  uint32_t elapsed=millis()-toastStart;
  int offset=0;
  if(settings.animations&&toastText.length())  {
    if(elapsed<150)offset=25-elapsed*25/150;
    else if(elapsed>2350)offset=min((uint32_t)25,(elapsed-2350)*25/250);
  }
  if(editing)display.renderKeyboard(keyboard,settings.theme);
  else display.render(current,status,toastText,toastType,settings.theme,settings.statusBar,settings.debugOverlay,offset);
  dirty=false;
}
void UI::update()  {
  uint32_t elapsed=millis()-toastStart;
  if(settings.animations&&toastText.length()&&(elapsed<160||elapsed>2350)&&millis()-animationAt>=16)  {
    animationAt=millis();
    dirty=true;
  }
  if(toastText.length()&&millis()-toastStart>2600)  {
    toastText="";
    dirty=true;
  }
}
void UI::toast(const String& text,ToastType type)  {
  toastText=text;
  toastType=type;
  toastStart=millis();
  dirty=true;
}
void UI::rows(const String& title,const String& text,bool push)  {
  MenuPage p;
  p.title=title;
  p.icon=Icon::Info;
  int start=0;
  for(int i=0;i<=(int)text.length()&&p.items.size()<64;i++)if(i==(int)text.length()||text[i]=='\n')  {
    String line=text.substring(start,i);
    if(line.isEmpty())p.items.push_back(  {
      " ","",Icon::Info
    }
    );
    else for(size_t j=0;j<line.length()&&p.items.size()<64;j+=31)p.items.push_back(  {
      line.substring(j,j+31),"",Icon::Info
    }
    );
    start=i+1;
  }
  page(std::move(p),push);
}
void UI::message(const String& title,const String& text)  {
  rows(title,text);
  current.items.push_back(  {
    "OK","",Icon::Info,[this]  {
      back();
    }
  }
  );
}
void UI::confirm(const String& title,const String& text,Action yes)  {
  MenuPage p;
  p.title=title;
  p.icon=Icon::Info;
  p.items=  {
    {
      "No",text,Icon::Info,[this]  {
        back();
      }
    },  {
      "Yes",text,Icon::Info,[this,yes]  {
        back();
        if(yes)yes();
      }
    }
  };
  page(std::move(p));
}
void UI::progress(const String& title,const String& detail,Action cancel)  {
  MenuPage p;
  p.title=title;
  p.icon=Icon::Tool;
  p.items=  {
    {
      detail,"Working...",Icon::Tool
    },  {
      "Cancel","",Icon::Info,[this]  {
        back();
      }
    }
  };
  p.onBack=[this,cancel]  {
    if(cancel)cancel();
    current.onBack=nullptr;
    back();
  };
  page(std::move(p));
}
void UI::textInput(const String& title,const String& initial,std::function<void(String)> done,int maximum,bool password,int mode) {
  keyboard.begin(title.c_str(),initial.c_str(),maximum,password,mode);
  inputDone=std::move(done);
  editing=true;
  display.invalidate();
  dirty=true;
}
void UI::numberInput(const String& title,uint32_t initial,uint32_t low,uint32_t high,std::function<void(uint32_t)> done,bool hex)  {
  textInput(title,String(initial,hex?16:10),[this,low,high,done,hex](String value)  {
    char* end=nullptr;unsigned long v=strtoul(value.c_str(),&end,hex?16:10);if(value.isEmpty()||*end||v<low||v>high)  {
      toast("Value out of range",ToastType::Error);return;
    }
    done(v);
  },10,false,hex?3:2);
}
