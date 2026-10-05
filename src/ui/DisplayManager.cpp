#include "DisplayManager.h"
#include "../pins.h"
#include "../core/Version.h"
#include "CyrillicFont.h"
#include <cstring>
void DisplayManager::screensaver(uint8_t mode,uint32_t now,uint8_t theme) {
  if(now-saverAt<40)return;
  saverAt=now;const auto& t=ThemeManager::get(theme);
  if(saverMode!=mode||invalid) {
    saverMode=mode;tft.fillScreen(t.background);invalid=false;
    for(int i=0;i<40;i++)starX[i]=starY[i]=-1;
    pipeX=tft.width()/2;pipeY=tft.height()/2;
  }
  saverSeed=saverSeed*1664525+1013904223;
  if(mode==1) {
    if((saverSeed%11)==0)pipeDirection=(saverSeed>>16)%4;
    int x=pipeX+(pipeDirection==0?6:pipeDirection==2?-6:0);
    int y=pipeY+(pipeDirection==1?6:pipeDirection==3?-6:0);
    if(x<3||y<3||x>=tft.width()-3||y>=tft.height()-3) { pipeDirection=(pipeDirection+2)%4;return; }
    tft.drawLine(pipeX,pipeY,x,y,t.accent);tft.drawLine(pipeX+1,pipeY+1,x+1,y+1,t.accent);
    pipeX=x;pipeY=y;
    if(now%30000<40)tft.fillScreen(t.background);
  } else if(mode==2) {
    for(int i=0;i<40;i++) {
      if(starX[i]>=0)tft.fillCircle(starX[i],starY[i],1,t.background);
      int r=(now/18+i*29)%512;
      int x=tft.width()/2+(((i*73+17)%201)-100)*r/200;
      int y=tft.height()/2+(((i*131+31)%201)-100)*r/150;
      starX[i]=starY[i]=-1;
      if(x>0&&y>0&&x<tft.width()&&y<tft.height()) { starX[i]=x;starY[i]=y;tft.fillCircle(x,y,1,r>250?t.foreground:t.muted); }
    }
  } else if(mode==3) {
    int col=(saverSeed>>8)%20,x=col*tft.width()/20;
    int y=(now/50+col*7)%(tft.height()/9)*9;
    tft.fillRect(x,(y+tft.height()-45)%tft.height(),6,9,t.background);
    print(String((saverSeed>>16)%10),x,y,t.accent);
  }
}
void DisplayManager::notification(const String& text,uint8_t theme) {
  const auto& t=ThemeManager::get(theme);int y=tft.height()-82;
  tft.fillRoundRect(6,y,tft.width()-12,74,5,t.panel);
  tft.drawRoundRect(6,y,tft.width()-12,74,5,t.accent);
  print("Notification",14,y+8,t.accent);
  int chars=(tft.width()-28)/6;
  std::string value=text.c_str();size_t offset=0;
  for(int row=0;row<2;row++) {
    size_t start=offset;
    for(int col=0;col<chars&&offset<value.size();col++)KeyboardModel::next(value,offset);
    print(value.substr(start,offset-start).c_str(),14,y+28+row*18,t.foreground);
  }
}
void DisplayManager::renderCalculator(const CalculatorModel& model,uint8_t theme) {
  const auto& t=ThemeManager::get(theme);
  if(calculatorVisible&&!invalid&&themeIndex==theme&&model.expression==previousCalculator.expression&&model.result==previousCalculator.result&&model.selected==previousCalculator.selected&&model.scientific==previousCalculator.scientific&&model.degrees==previousCalculator.degrees)return;
  bool full=invalid||!calculatorVisible||themeIndex!=theme||model.scientific!=previousCalculator.scientific;
  if(full)tft.fillScreen(t.background);
  calculatorVisible=true;themeIndex=theme;invalid=false;
  tft.fillRect(0,0,tft.width(),82,t.background);
  print(model.scientific?"Scientific calculator":"Calculator",8,8,t.accent);
  int chars=(tft.width()-16)/6;
  std::string tail=model.expression.substr(model.expression.size()>size_t(chars)?model.expression.size()-chars:0);
  print(tail.c_str(),8,30,t.foreground);print(model.result.c_str(),8,55,t.accent);
  int rows=model.count()/5,h=(tft.height()-106)/rows,w=(tft.width()-8)/5;
  for(int i=0;i<model.count();i++) {
    if(!full&&i!=model.selected&&i!=previousCalculator.selected&&model.degrees==previousCalculator.degrees)continue;
    int x=4+(i%5)*w,y=84+(i/5)*h;
    tft.fillRoundRect(x+1,y+1,w-3,h-3,3,i==model.selected?t.selection:t.panel);
    print(model.label(i),x+4,y+h/2-3,i==model.selected?t.accent:t.foreground);
  }
  print("Arrows / OK key | Hold OK exit",8,tft.height()-14,t.muted);
  previousCalculator=model;
}
DisplayManager::DisplayManager():tft(&SPI,Pins::TFT_CS,Pins::TFT_DC,Pins::TFT_RST)  {
}
void DisplayManager::begin(uint8_t r)  {
  tft.init(240,320);
  tft.setRotation(r);
  tft.setTextWrap(false);
  tft.fillScreen(0);
}
void DisplayManager::rotation(uint8_t r)  {
  tft.setRotation(r);
  invalidate();
}
void DisplayManager::print(const String& text,int x,int y,uint16_t color,uint8_t size)  {
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.setCursor(x,y);
  bool unicode=false;
  for(size_t i=0;i<text.length();i++)if((uint8_t)text[i]>=128) { unicode=true; break; }
  if(unicode) {
    std::string value=text.c_str();
    size_t offset=0;
    while(offset<value.size()) {
      uint32_t cp=KeyboardModel::next(value,offset);
      const uint8_t* bitmap=CyrillicFont::glyph(cp);
      if(bitmap) {
        for(int row=0;row<7;row++)for(int col=0;col<5;col++)
          if(bitmap[row]&(1<<(4-col)))tft.fillRect(x+col*size,y+row*size,size,size,color);
      } else {
        tft.setCursor(x,y);
        tft.print(String(std::string(1,cp<128?(char)cp:'?').c_str()));
      }
      x+=6*size;
    }
    return;
  }
  tft.print(text);
}
void DisplayManager::splash()  {
  tft.fillScreen(0);
  print("DMultiTool",22,92,0xF81F,3);
  print("v" FW_VERSION,24,156,0x9CD5);
  tft.drawFastHLine(24,184,tft.width()-48,0xF81F);
  invalidate();
}
void DisplayManager::bootStatus(const String& s)  {
  tft.fillRect(20,200,tft.width()-40,48,0);
  print(s.substring(0,32),24,214,0xFFFF);
  invalidate();
}
void DisplayManager::sleep(bool on)  {
  if(asleep==on)return;
  asleep=on;
  tft.enableDisplay(!on);
  if(!on)invalidate();
}
void DisplayManager::icon(Icon id,int x,int y,uint16_t c)  {
  if(id==Icon::WiFi)  {
    for(int i=0;i<3;i++)tft.drawFastHLine(x+2-i*2,y+3+i*4,4+i*4,c);
    tft.fillCircle(x+4,y+15,1,c);
  }
  else if(id==Icon::BLE)  {
    tft.drawLine(x+6,y,x+6,y+16,c);
    tft.drawLine(x+6,y,x+12,y+5,c);
    tft.drawLine(x+12,y+5,x,y+13,c);
    tft.drawLine(x,y+3,x+12,y+11,c);
    tft.drawLine(x+12,y+11,x+6,y+16,c);
  }
  else if(id==Icon::NFC)  {
    tft.drawRoundRect(x,y,16,16,3,c);
    tft.drawRoundRect(x+4,y+3,8,10,2,c);
  }
  else if(id==Icon::Folder)  {
    tft.drawRect(x,y+4,17,12,c);
    tft.drawRect(x+1,y,7,5,c);
  }
  else if(id==Icon::IR)  {
    tft.drawCircle(x+8,y+8,7,c);
    tft.drawCircle(x+8,y+8,3,c);
  }
  else if(id==Icon::Settings||id==Icon::Tool)  {
    tft.drawCircle(x+8,y+8,6,c);
    tft.drawCircle(x+8,y+8,2,c);
    tft.drawFastHLine(x,y+8,17,c);
  }
  else if(id==Icon::Game) {
    tft.drawRoundRect(x,y+3,18,12,4,c);
    tft.drawFastHLine(x+3,y+9,6,c);
    tft.drawLine(x+6,y+6,x+6,y+11,c);
    tft.fillCircle(x+13,y+7,1,c);tft.fillCircle(x+15,y+11,1,c);
  }
  else if(id==Icon::Python) {
    tft.drawRoundRect(x+2,y,12,9,3,c);
    tft.drawRoundRect(x+5,y+7,12,9,3,c);
    tft.fillCircle(x+5,y+3,1,c);tft.fillCircle(x+14,y+13,1,c);
  }
  else if(id==Icon::Script)  {
    print(">_",x,y+4,c);
  }
  else  {
    tft.drawRoundRect(x,y,16,16,2,c);
    print(id==Icon::Info?"i":"+",x+5,y+4,c);
  }
}
void DisplayManager::render(const MenuPage& page,const Status& s,const String& toast,ToastType type,uint8_t theme,bool bar,bool debug,int toastOffset)  {
  if(asleep)return;
  if(calculatorVisible) { calculatorVisible=false;invalid=true; }
  if(pythonVisible) { pythonVisible=false;invalid=true; }
  if(gameVisible) { gameVisible=false;invalid=true; }
  if(keyboardVisible) { keyboardVisible=false; invalid=true; }
  const Theme& t=ThemeManager::get(theme);
  if(theme!=themeIndex||bar!=barBefore||oldColumns!=page.columns||gridBefore!=page.grid)  {
    invalid=true;
    themeIndex=theme;
    barBefore=bar;
    gridBefore=page.grid;
  }
  if(invalid)  {
    tft.fillScreen(t.background);
    for(auto& line:previous)line="";
    header=footer=focus="";
    oldTop=-1;
  }
  String h=String("DMT ")+(s.connected?"W* ":s.wifi?"W ":"- ")+(s.bleConnected?"B* ":s.ble?"B ":"- ")+(s.sd?"SD ":"-- ")+(s.nfc?"N ":"- ")+String(s.heap/1024)+"k "+String(s.uptime/60)+"m";
  String headerKey=h+String((int)page.icon);
  if(bar&&(invalid||header!=headerKey))  {
    tft.fillRect(0,0,tft.width(),26,t.panel);
    icon(page.icon,6,5,t.accent);
    print(h,28,9,t.foreground);
    header=headerKey;
  }
  int titleY=bar?34:8;
  String f=page.title+"|"+(page.items.empty()?String():page.items[page.selected].label);
  if(invalid||focus!=f)  {
    tft.fillRect(0,titleY-2,tft.width(),46,t.background);
    String big=page.launcher&&!page.items.empty()?page.items[page.selected].label:page.title;
    print(big.substring(0,(tft.width()-16)/12),8,titleY,t.accent,2);
    print(page.launcher?page.title.substring(0,35):page.items.empty()?"":(page.grid?page.items[page.selected].detail:page.items[page.selected].label).substring(0,35),8,titleY+25,t.muted);
    focus=f;
  }
  int begin=titleY+50,columns=page.grid?3:page.columns;
  int rowH=page.grid?(tft.height()-begin-28)/4:columns>1?38:30;
  int visibleRows=page.grid?4:min(12,(tft.height()-begin-28)/rowH);
  int visible=page.grid?12:min(32,visibleRows*columns);
  int top=page.grid?(page.selected/12)*12:max(0,page.selected/columns-visibleRows+1)*columns;
  for(int i=0;i<visible;i++)  {
    int n=top+i;
    bool selected=n==page.selected&&n<(int)page.items.size();
    String line=n<(int)page.items.size()?page.items[n].label+"|"+page.items[n].detail+"|"+String((int)page.items[n].icon)+"|"+String(page.items[n].enabled)+"|"+String(page.items[n].swatch):"";
    if(invalid||top!=oldTop||line!=previous[i]||selected!=selectedBefore[i])  {
      int y=begin+(i/columns)*rowH;
      int x=(i%columns)*(tft.width()-8)/columns;
      int width=(tft.width()-8)/columns;
      tft.fillRect(x,y,width,rowH,t.background);
      if(n<(int)page.items.size())  {
        auto& item=page.items[n];
        uint16_t c=item.enabled?t.foreground:t.muted;
        tft.fillRoundRect(x+4,y+1,width-6,rowH-3,4,selected?t.selection:t.panel);
        if(selected)tft.fillRect(x+4,y+4,3,rowH-9,t.accent);
        if(page.grid) {
          if(item.swatch)tft.fillRoundRect(x+10,y+4,width-20,16,3,item.swatch);
          else icon(item.icon,x+width/2-8,y+4,selected?t.accent:c);
          int chars=(width-12)/6;
          String label=item.label.substring(0,chars);
          print(label,x+(width-(int)label.length()*6)/2,y+rowH-12,c);
        }
        else if(columns>1)  {
          if(item.swatch)tft.fillRoundRect(x+10,y+5,width-20,9,2,item.swatch);
          print(item.label.substring(0,(width-12)/6),x+9,y+23,c);
        }
        else  {
          icon(item.icon,x+12,y+6,selected?t.accent:c);
          print(item.label.substring(0,(width-46)/6),x+36,y+(item.detail.length()?5:11),c);
          if(item.detail.length())print(item.detail.substring(0,(width-46)/6),x+36,y+18,t.muted);
        }
      }
      previous[i]=line;
      selectedBefore[i]=selected;
    }
  }
  if(invalid||top!=oldTop||oldCount!=(int)page.items.size())  {
    tft.fillRect(tft.width()-5,begin,3,visibleRows*rowH,t.panel);
    if(page.items.size()>(size_t)visible)  {
      int height=max(8,visible*visibleRows*rowH/(int)page.items.size());
      int y=begin+top*(visibleRows*rowH-height)/max(1,(int)page.items.size()-visible);
      tft.fillRect(tft.width()-5,y,3,height,t.accent);
    }
  }
  String bottom=toast.length()?toast:debug?"FPS "+String(s.fps)+" Up "+String(s.uptime)+"s "+String(s.heap/1024)+"k":page.hint;
  if(invalid||footer!=bottom||toastText!=toast||oldToastOffset!=toastOffset)  {
    tft.fillRect(0,tft.height()-25,tft.width(),25,toast.length()?(type==ToastType::Error?t.error:type==ToastType::Success?(theme==2?0xAFE5:0x0340):type==ToastType::Warning?(theme==2?0xFFE0:0xA300):t.selection):t.panel);
    if(toast.length())tft.fillRect(0,tft.height()-25,tft.width(),toastOffset,t.panel);
    print(bottom.substring(0,tft.width()/6-2),6,tft.height()-16+(toast.length()?toastOffset:0),t.foreground);
    footer=bottom;
    toastText=toast;
  }
  oldTop=top;
  oldColumns=columns;
  oldCount=page.items.size();
  oldToastOffset=toastOffset;
  invalid=false;
}

void DisplayManager::keyboardControl(int control,int x,int y,uint16_t c,bool active) {
  if(control==KeyboardModel::Lang) {
    tft.drawCircle(x+10,y+10,9,c);
    tft.drawRoundRect(x+6,y+1,8,18,4,c);
    tft.drawFastHLine(x+1,y+10,18,c);
    tft.drawFastHLine(x+3,y+5,14,c);
    tft.drawFastHLine(x+3,y+15,14,c);
  } else if(control==KeyboardModel::Enter) {
    tft.drawLine(x+18,y+2,x+18,y+12,c);
    tft.drawLine(x+18,y+12,x+2,y+12,c);
    tft.drawLine(x+2,y+12,x+7,y+7,c);
    tft.drawLine(x+2,y+12,x+7,y+17,c);
  } else if(control==KeyboardModel::Backspace) {
    tft.drawLine(x,y+10,x+6,y+3,c);
    tft.drawLine(x,y+10,x+6,y+17,c);
    tft.drawLine(x+6,y+3,x+20,y+3,c);
    tft.drawLine(x+6,y+17,x+20,y+17,c);
    tft.drawLine(x+20,y+3,x+20,y+17,c);
    tft.drawLine(x+10,y+7,x+16,y+13,c);
    tft.drawLine(x+10,y+13,x+16,y+7,c);
  } else {
    tft.drawLine(x+10,y,x,y+10,c);
    tft.drawLine(x+10,y,x+20,y+10,c);
    tft.drawLine(x,y+10,x+5,y+10,c);
    tft.drawLine(x+20,y+10,x+15,y+10,c);
    tft.drawRect(x+5,y+10,10,9,c);
    if(active)tft.fillRect(x+7,y+12,6,5,c);
  }
}
void DisplayManager::renderKeyboard(const KeyboardModel& k,uint8_t theme) {
  if(asleep)return;
  if(pythonVisible) { pythonVisible=false;invalid=true; }
  if(gameVisible) { gameVisible=false;invalid=true; }
  const Theme& t=ThemeManager::get(theme);
  bool full=invalid||!keyboardVisible||keyboardTheme!=theme;
  bool layoutChanged=full||previousKeyboard.language!=k.language||previousKeyboard.shift!=k.shift;
  bool selectionChanged=previousKeyboard.selected!=k.selected;
  if(full) {
    tft.fillScreen(t.background);
    print(String(k.title.c_str()).substring(0,(tft.width()-16)/6),8,10,t.muted);
  }
  if(full||previousKeyboard.value!=k.value||previousKeyboard.password!=k.password||layoutChanged) {
    tft.fillRoundRect(8,28,tft.width()-16,54,5,t.panel);
    print("Ввод:",16,55,t.muted);
    String badge=k.language==KeyboardModel::Language::English?"EN":k.language==KeyboardModel::Language::Russian?"РУ":"123 #";
    print(badge,tft.width()-52,36,t.accent);
    int capacity=(tft.width()-68)/12;
    std::string shown;
    if(k.password)shown=std::string(KeyboardModel::count(k.value),'*');
    else {
      size_t offset=0,skip=KeyboardModel::count(k.value);
      skip=skip>(size_t)capacity?skip-capacity:0;
      while(skip--)KeyboardModel::next(k.value,offset);
      shown=k.value.substr(offset);
    }
    if(shown.size()>(size_t)capacity&&k.password)shown=shown.substr(shown.size()-capacity);
    if(shown.empty())shown="____________";
    print(shown.c_str(),52,50,t.foreground,2);
  }
  const int top=92, cellW=(tft.width()-16)/9;
  const int rowH=min(42,(tft.height()-top-52)/k.rows());
  if(layoutChanged)tft.fillRect(0,86,tft.width(),tft.height()-132,t.background);
  for(int i=0;i<k.characters();i++) {
    if(!layoutChanged && (!selectionChanged||(i!=k.selected && i!=previousKeyboard.selected)))continue;
    int x=8+(i%9)*cellW,y=top+(i/9)*rowH;
    bool selected=i==k.selected;
    tft.fillRoundRect(x,y,cellW-2,rowH-3,3,selected?t.selection:t.panel);
    uint16_t color=selected?t.accent:t.foreground;
    uint32_t cp=k.character(i);
    if(cp==' ') {
      int yy=y+(rowH-3)/2+4;
      tft.drawFastHLine(x+5,yy,cellW-12,color);
      tft.drawLine(x+5,yy,x+5,yy-4,color);
      tft.drawLine(x+cellW-8,yy,x+cellW-8,yy-4,color);
    } else print(KeyboardModel::utf8(cp).c_str(),x+(cellW-12)/2,y+(rowH-17)/2,color,2);
  }
  int controlW=(tft.width()-16)/4,controlY=tft.height()-44;
  for(int i=0;i<4;i++) {
    int index=k.characters()+i;
    if(!layoutChanged && (!selectionChanged||(index!=k.selected && index!=previousKeyboard.selected)))continue;
    int x=8+i*controlW;
    tft.fillRoundRect(x,controlY,controlW-3,34,5,index==k.selected?t.selection:t.panel);
    keyboardControl(i,x+(controlW-22)/2,controlY+6,index==k.selected?t.accent:t.foreground,k.shift);
  }
  previousKeyboard=k;
  keyboardTheme=theme;
  keyboardVisible=true;
  invalid=false;
}

void DisplayManager::renderGame(const Games::Board& b,uint8_t theme,bool paused) {
  if(asleep||!b.width||!b.height)return;
  if(pythonVisible) { pythonVisible=false;invalid=true; }
  const Theme& t=ThemeManager::get(theme);
  bool full=invalid||!gameVisible||keyboardVisible||gameTheme!=theme||previousGame.kind!=b.kind||previousGame.over!=b.over||gamePaused!=paused;
  const char* title=b.kind==Games::Board::Snake?"Snake":b.kind==Games::Board::Minesweeper?"Minesweeper":"Tetris";
  int cell=min((tft.width()-20)/(int)b.width,(tft.height()-86)/(int)b.height);
  cell=min(cell,24);
  int left=(tft.width()-b.width*cell)/2,top=60;
  if(full) {
    tft.fillScreen(t.background);
    print(title,10,10,t.accent,2);
    print("HOLD OK: exit",tft.width()-90,12,t.muted);
    tft.drawRect(left-1,top-1,b.width*cell+2,b.height*cell+2,t.muted);
    const char* hint=b.kind==Games::Board::Snake?"Arrows steer  OK pause":b.kind==Games::Board::Minesweeper?"OK open  2xOK flag":"UP rotate DOWN fall OK drop";
    print(hint,8,tft.height()-16,t.muted);
  }
  if(full||previousGame.score!=b.score||previousGame.detail!=b.detail) {
    tft.fillRect(8,35,tft.width()-16,17,t.background);
    print("Score "+String(b.score)+(b.kind==Games::Board::Minesweeper?"  Mines "+String(b.detail):b.kind==Games::Board::Tetris?"  Lines "+String(b.detail):""),8,38,t.foreground);
  }
  static constexpr uint16_t colors[]={0,0x07FF,0xFFE0,0xF81F,0xFD20,0x001F,0x07E0,0xF800};
  for(int i=0;i<b.width*b.height;i++) {
    if(!full&&b.cells[i]==previousGame.cells[i]&&(b.cursor==previousGame.cursor|| (i!=b.cursor&&i!=previousGame.cursor)))continue;
    int x=left+(i%b.width)*cell,y=top+(i/b.width)*cell;
    uint8_t value=b.cells[i];uint16_t color=t.panel;
    if(b.kind==Games::Board::Snake)color=value==1?0x05A0:value==2?0x07E0:value==3?0xF800:t.panel;
    else if(b.kind==Games::Board::Tetris)color=value?colors[value]:t.panel;
    else color=value>=2?t.background:t.selection;
    tft.fillRect(x,y,cell,cell,t.background);
    tft.fillRect(x+1,y+1,cell-2,cell-2,color);
    if(b.kind==Games::Board::Minesweeper) {
      if(value==1) {
        tft.drawLine(x+cell/2,y+4,x+cell/2,y+cell-4,t.accent);
        tft.drawFastHLine(x+cell/2,y+4,cell/3,t.accent);
      } else if(value==12)tft.fillCircle(x+cell/2,y+cell/2,max(2,cell/4),t.error);
      else if(value>2)print(String(value-2),x+(cell-6)/2,y+(cell-8)/2,t.accent);
      if(i==b.cursor)tft.drawRect(x,y,cell,cell,t.accent);
    }
  }
  if((b.over||paused)&&full) {
    int y=top+b.height*cell/2-24;
    tft.fillRoundRect(10,y,tft.width()-20,50,5,t.selection);
    print(paused?"PAUSED":b.won?"YOU WIN":"GAME OVER",24,y+8,t.foreground,2);
    print(paused?"OK resume / hold OK exit":"OK restart / hold OK exit",20,y+33,t.muted);
  }
  previousGame=b;gameVisible=true;keyboardVisible=false;gameTheme=theme;gamePaused=paused;invalid=false;
}

void DisplayManager::renderPython(const PythonView& view,uint8_t theme) {
  if(asleep)return;
  const Theme& t=ThemeManager::get(theme);
  bool full=invalid||!pythonVisible||gameVisible||keyboardVisible||pythonTheme!=theme;
  bool canvas=full||memcmp(previousPython.rectangles.data(),view.rectangles.data(),sizeof(view.rectangles))||previousPython.rectanglesCount!=view.rectanglesCount;
  if(full)tft.fillScreen(t.background);
  if(full||strcmp(view.title,previousPython.title)||strcmp(view.status,previousPython.status)) {
    tft.fillRect(0,0,tft.width(),54,t.background);
    print(String(view.title).substring(0,(tft.width()-16)/12),8,10,t.accent,2);
    print(String(view.status).substring(0,(tft.width()-16)/6),8,36,t.muted);
  }
  int available=tft.height()-112;
  int visible=max(1,available/26),top=max(0,view.selected-visible+1);
  int oldTop=max(0,previousPython.selected-visible+1);
  canvas=canvas||top!=oldTop;
  if(canvas) {
    tft.fillRect(0,56,tft.width(),available+8,t.background);
    for(int i=0;i<view.rectanglesCount;i++) {
      auto& r=view.rectangles[i];
      tft.fillRect(8+r.x1,60+r.y1,r.x2-r.x1,r.y2-r.y1,r.color);
    }
  }
  // Scroll the compact pack() stack so the selected button remains visible.
  for(int slot=0;slot<visible;slot++) {
    int i=top+slot;
    bool changed=canvas||top!=oldTop||view.count!=previousPython.count;
    if(i<view.count)changed=changed||strcmp(view.widgets[i].text,previousPython.widgets[i].text)||view.widgets[i].button!=previousPython.widgets[i].button||((i==view.selected)!=(i==previousPython.selected));
    if(!changed)continue;
    int y=60+slot*26;
    if(i>=view.count)continue;
    auto& widget=view.widgets[i];
    tft.fillRoundRect(8,y,tft.width()-16,23,4,i==view.selected?t.selection:t.panel);
    print(String(widget.text).substring(0,(tft.width()-32)/6),16,y+8,widget.button?t.accent:t.foreground);
  }
  if(full||strcmp(view.console,previousPython.console)) {
    tft.fillRect(0,tft.height()-44,tft.width(),44,t.panel);
    print(String(view.console).substring(0,(tft.width()-16)/6),8,tft.height()-35,t.foreground);
    print("Hold OK -> confirm exit",8,tft.height()-15,t.muted);
  }
  previousPython=view;pythonTheme=theme;pythonVisible=true;gameVisible=keyboardVisible=false;invalid=false;
}
