#include "DisplayManager.h"
#include "../games/ArcadeModels.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace {
constexpr int Patch=16;
const uint16_t Colors[]={0,0x07FF,0xFFE0,0xF81F,0xFD20,0x001F,0x07E0,0xF800};
const uint16_t ArcadeColors[]={0,0x07FF,0x07E0,0xFFE0,0xF81F,0xFD20,0x001F,0xF800};
void text(GFXcanvas16& canvas,const String& label,int x,int y,uint16_t color,int size=1) {
  canvas.setTextSize(size);canvas.setTextColor(color);canvas.setCursor(x,y);canvas.print(label);
}
void sprite(GFXcanvas16& canvas,const Games::Board::Sprite& a,int width,int height,int ox,int oy,uint16_t accent) {
  // Coordinates can be outside the viewport. Canvas clips the final pixels.
  auto px=[&](int x){return x*width/160-ox;};
  auto py=[&](int y){return y*height/180-oy;};
  int x=px(a.x),y=py(a.y),w=std::max(1,px(a.x+a.w)-x),h=std::max(1,py(a.y+a.h)-y);
  uint16_t color=ArcadeColors[a.color%8];
  using S=Games::Board::Sprite;
  if(a.type==S::Ball)canvas.fillCircle(x+w/2,y+h/2,std::max(1,w/2),color);
  else if(a.type==S::Ship) {
    float radians=a.angle*3.14159265f/180;int cx=a.x+a.w/2,cy=a.y+a.h/2;
    int xs[3],ys[3];
    for(int k=0;k<3;k++) { float d=radians+(k==0?0:k==1?2.5f:-2.5f);xs[k]=px(cx+int(cosf(d)*7));ys[k]=py(cy+int(sinf(d)*7)); }
    for(int k=0;k<3;k++)canvas.drawLine(xs[k],ys[k],xs[(k+1)%3],ys[(k+1)%3],color);
  } else if(a.type==S::Rock) {
    int cx=a.x+a.w/2,cy=a.y+a.h/2;int xs[8],ys[8];
    for(int k=0;k<8;k++) { float d=k*3.14159265f/4;float radius=(k%3==0?.8f:1.f)*a.w/2;xs[k]=px(cx+int(cosf(d)*radius));ys[k]=py(cy+int(sinf(d)*radius)); }
    for(int k=0;k<8;k++)canvas.drawLine(xs[k],ys[k],xs[(k+1)%8],ys[(k+1)%8],color);
  } else if(a.type==S::Bird) {
    canvas.fillRoundRect(x,y,w,h,2,color);canvas.fillRect(x+w/2,y+h/3,std::max(1,w/4),std::max(1,h/4),0);canvas.drawLine(x,y+h/2,x+w/3,y+h/3,accent);
  } else if(a.type==S::Dino) {
    canvas.fillRect(x,y+h/3,std::max(1,w*2/3),std::max(1,h*2/3),color);canvas.fillRect(x+w/2,y,std::max(1,w/2),std::max(1,h/2),color);canvas.fillRect(x+w*3/4,y+1,1,1,0);
  } else if(a.type==S::Alien) {
    canvas.fillRect(x+1,y+h/3,std::max(1,w-2),std::max(1,h/2),color);canvas.fillRect(x+w/4,y,std::max(1,w/2),h,color);
    canvas.fillRect(x+w/3,y+h/3,2,2,0);canvas.fillRect(x+w*2/3,y+h/3,2,2,0);
  } else canvas.fillRect(x,y,w,h,color);
}
}
void DisplayManager::renderGame(const Games::Board& b,uint8_t theme,bool paused) {
  if(asleep||!b.width||!b.height)return;
  bool full=invalid||!gameVisible||keyboardVisible||pythonVisible||calculatorVisible||gameTheme!=theme||previousGame.kind!=b.kind||previousGame.over!=b.over||previousGame.won!=b.won||gamePaused!=paused;
  const auto& t=ThemeManager::get(theme);
  bool arcade=b.kind>=Games::Board::Pong,puzzle=b.kind==Games::Board::Puzzle2048;
  int cell=std::min((tft.width()-20)/int(b.width),(tft.height()-86)/int(b.height));
  cell=std::min(cell,puzzle?80:24);
  int width=b.width*cell,height=b.height*cell;
  if(puzzle) { width=std::min(tft.width()-20,tft.height()-86);height=width;cell=width/4; }
  if(arcade) { width=std::min(tft.width()-20,(tft.height()-86)*160/180);height=width*180/160; }
  int left=(tft.width()-width)/2,top=60;
  if(full) {
    tft.fillScreen(t.background);
    print(Games::title(b.kind),arcade||puzzle?8:10,arcade||puzzle?9:10,t.accent,2);
    if(!arcade&&!puzzle)print("HOLD OK: exit",tft.width()-90,12,t.muted);
    tft.drawRect(left-1,top-1,width+2,height+2,t.muted);
    print(Games::controls(b.kind),6,tft.height()-16,t.muted);
  }
  if(full||b.score!=previousGame.score||b.detail!=previousGame.detail) {
    tft.fillRect(8,35,tft.width()-16,17,t.background);
    String extra=b.kind==Games::Board::Minesweeper?"  Mines "+String(b.detail):b.kind==Games::Board::Tetris?"  Lines "+String(b.detail):b.kind==Games::Board::Pong?"  AI "+String(b.detail):b.kind==Games::Board::Breakout||b.kind==Games::Board::SpaceInvaders||b.kind==Games::Board::Asteroids?"  Lives "+String(b.detail):"";
    print("Score "+String(b.score)+extra,8,38,t.foreground);
  }
  // Mark both old and new bounds, including objects removed/compacted in the array.
  // Composite every overlapping object in RAM, then send each finished patch once.
  const int columns=(width+Patch-1)/Patch,rows=(height+Patch-1)/Patch;
  std::array<bool,400> dirty{};
  auto mark=[&](int x,int y,int w,int h) {
    int x0=std::max(0,x),y0=std::max(0,y),x1=std::min(width-1,x+w-1),y1=std::min(height-1,y+h-1);
    if(x0>x1||y0>y1)return;
    for(int r=y0/Patch;r<=y1/Patch;r++)for(int c=x0/Patch;c<=x1/Patch;c++)dirty[r*columns+c]=true;
  };
  auto bounds=[&](const Games::Board::Sprite& a) { mark(a.x*width/160-4,a.y*height/180-4,a.w*width/160+10,a.h*height/180+10); };
  if(full)mark(0,0,width,height);
  else if(arcade) {
    for(int i=0;i<std::max(b.spriteCount,previousGame.spriteCount);i++) {
      if(i<b.spriteCount&&i<previousGame.spriteCount&&b.sprites[i]==previousGame.sprites[i])continue;
      if(i<previousGame.spriteCount)bounds(previousGame.sprites[i]);
      if(i<b.spriteCount)bounds(b.sprites[i]);
    }
  } else {
    for(int i=0;i<b.width*b.height;i++)if(b.cells[i]!=previousGame.cells[i]||(b.cursor!=previousGame.cursor&&(i==b.cursor||i==previousGame.cursor)))mark((i%b.width)*cell,(i/b.width)*cell,cell,cell);
  }
  for(int row=0;row<rows;row++)for(int col=0;col<columns;col++)if(dirty[row*columns+col]) {
    int ox=col*Patch,oy=row*Patch,w=std::min(Patch,width-ox),h=std::min(Patch,height-oy);
    auto& canvas=gameCanvas;canvas.fillScreen(t.background);
    if(arcade) {
      for(int i=0;i<b.spriteCount;i++) {
        const auto& a=b.sprites[i];int x=a.x*width/160,y=a.y*height/180,sw=a.w*width/160,sh=a.h*height/180;
        if(x+sw+4<ox||x-4>=ox+w||y+sh+4<oy||y-4>=oy+h)continue;
        sprite(canvas,a,width,height,ox,oy,t.accent);
      }
    } else {
      for(int i=0;i<b.width*b.height;i++) {
        int x=(i%b.width)*cell-ox,y=(i/b.width)*cell-oy;if(x+cell<=0||x>=w||y+cell<=0||y>=h)continue;
        uint8_t v=b.cells[i];uint16_t color=t.panel;
        if(puzzle) {
          canvas.fillRoundRect(x+2,y+2,cell-4,cell-4,3,v?ArcadeColors[1+(v%7)]:t.panel);
          if(v) { String label=String(1u<<v);int size=cell>=48&&label.length()<=3?2:1;text(canvas,label,x+(cell-label.length()*6*size)/2,y+(cell-8*size)/2,0,size); }
          continue;
        }
        if(b.kind==Games::Board::Snake)color=v==1?0x05A0:v==2?0x07E0:v==3?0xF800:t.panel;
        else if(b.kind==Games::Board::Tetris)color=v?Colors[v]:t.panel;
        else color=v>=2?t.background:t.selection;
        canvas.fillRect(x+1,y+1,cell-2,cell-2,color);
        if(b.kind==Games::Board::Minesweeper) {
          if(v==1) { canvas.drawLine(x+cell/2,y+4,x+cell/2,y+cell-4,t.accent);canvas.drawFastHLine(x+cell/2,y+4,cell/3,t.accent); }
          else if(v==12)canvas.fillCircle(x+cell/2,y+cell/2,std::max(2,cell/4),t.error);
          else if(v>2)text(canvas,String(v-2),x+(cell-6)/2,y+(cell-8)/2,t.accent);
          if(i==b.cursor)canvas.drawRect(x,y,cell,cell,t.accent);
        }
      }
    }
    auto* pixels=canvas.getBuffer();
    // Pack the last partial column: drawRGBBitmap expects rows with stride w.
    if(w!=Patch)for(int y=1;y<h;y++)std::memmove(pixels+y*w,pixels+y*Patch,w*sizeof(uint16_t));
    tft.drawRGBBitmap(left+ox,top+oy,pixels,w,h);
  }
  if((b.over||paused)&&full) {
    int y=top+height/2-24;tft.fillRoundRect(10,y,tft.width()-20,50,5,t.selection);
    print(paused?"PAUSED":b.won?"YOU WIN":"GAME OVER",24,y+8,t.foreground,2);
    print(paused?"OK resume / hold OK exit":"OK restart / hold OK exit",20,y+33,t.muted);
  }
  previousGame=b;gameVisible=true;keyboardVisible=pythonVisible=calculatorVisible=false;gameTheme=theme;gamePaused=paused;invalid=false;
}
