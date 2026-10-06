#pragma once
#include "Arduino.h"
#include <algorithm>
#include <cmath>
class GFXcanvas16 {
protected:
  uint16_t* buffer=nullptr;bool buffer_owned=false;
  int width,height,cursorX=0,cursorY=0,textSize=1;uint16_t textColor=0xffff;
public:
  GFXcanvas16(uint16_t w,uint16_t h,bool allocate=true):buffer_owned(allocate),width(w),height(h) { if(allocate)buffer=new uint16_t[w*h]{}; }
  virtual ~GFXcanvas16(){if(buffer_owned)delete[] buffer;}
  uint16_t* getBuffer()const{return buffer;}
  void setTextWrap(bool){}
  void setCursor(int x,int y){cursorX=x;cursorY=y;}
  void setTextSize(int size){textSize=size;}
  void setTextColor(uint16_t color){textColor=color;}
  void drawPixel(int x,int y,uint16_t c){if(x>=0&&y>=0&&x<width&&y<height)buffer[y*width+x]=c;}
  void fillScreen(uint16_t c){std::fill(buffer,buffer+width*height,c);}
  void fillRect(int x,int y,int w,int h,uint16_t c){for(int yy=std::max(0,y);yy<std::min(height,y+h);yy++)for(int xx=std::max(0,x);xx<std::min(width,x+w);xx++)drawPixel(xx,yy,c);}
  void drawLine(int x,int y,int xx,int yy,uint16_t c){int dx=std::abs(xx-x),sx=x<xx?1:-1,dy=-std::abs(yy-y),sy=y<yy?1:-1,e=dx+dy;for(;;){drawPixel(x,y,c);if(x==xx&&y==yy)break;int n=2*e;if(n>=dy){e+=dy;x+=sx;}if(n<=dx){e+=dx;y+=sy;}}}
  void drawFastHLine(int x,int y,int w,uint16_t c){fillRect(x,y,w,1,c);}
  void drawRect(int x,int y,int w,int h,uint16_t c){drawLine(x,y,x+w-1,y,c);drawLine(x,y+h-1,x+w-1,y+h-1,c);drawLine(x,y,x,y+h-1,c);drawLine(x+w-1,y,x+w-1,y+h-1,c);}
  void fillCircle(int x,int y,int r,uint16_t c){for(int yy=std::max(0,y-r);yy<std::min(height,y+r+1);yy++)for(int xx=std::max(0,x-r);xx<std::min(width,x+r+1);xx++)if((xx-x)*(xx-x)+(yy-y)*(yy-y)<=r*r)drawPixel(xx,yy,c);}
  void fillRoundRect(int x,int y,int w,int h,int r,uint16_t c){r=std::min(r,std::min(w,h)/2);for(int yy=std::max(0,y);yy<std::min(height,y+h);yy++)for(int xx=std::max(0,x);xx<std::min(width,x+w);xx++){int cx=std::max(x+r,std::min(xx,x+w-r-1)),cy=std::max(y+r,std::min(yy,y+h-r-1));if((xx-cx)*(xx-cx)+(yy-cy)*(yy-cy)<=r*r)drawPixel(xx,yy,c);}}
  void print(const String& s){
    static const uint8_t digits[10][5]={{0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1e}};
    for(size_t i=0;i<s.length();i++){char c=s[i];if(c>='0'&&c<='9')for(int col=0;col<5;col++)for(int row=0;row<7;row++)if(digits[c-'0'][col]&(1<<row))fillRect(cursorX+col*textSize,cursorY+row*textSize,textSize,textSize,textColor);cursorX+=6*textSize;}
  }
};
