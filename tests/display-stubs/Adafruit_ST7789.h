#pragma once
#include "Arduino.h"
#include <vector>
#include <cstdio>
#include <fstream>
struct SPIClass{};
inline SPIClass SPI;
namespace MockTFT {
 struct Rect { int x,y,w,h; };
 inline std::vector<uint16_t> pixels;inline int rasterW=240,rasterH=320;
 inline std::vector<Rect> directFills,bitmapWrites;
 inline void fill(int x,int y,int w,int h,uint16_t c){for(int yy=std::max(0,y);yy<std::min(rasterH,y+h);yy++)for(int xx=std::max(0,x);xx<std::min(rasterW,x+w);xx++)pixels[yy*rasterW+xx]=c;}
 inline std::vector<std::string> shapes;inline bool enabled=true;
 inline std::string color(uint16_t c){char b[9];std::snprintf(b,sizeof(b),"#%02x%02x%02x",((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31);return b;}
 inline std::string escape(const std::string& s){std::string out;for(char c:s){if(c=='&')out+="&amp;";else if(c=='<')out+="&lt;";else if(c=='>')out+="&gt;";else if(c=='\"')out+="&quot;";else out+=c;}return out;}
 inline void write(const char* path,int w,int h){std::ofstream f(path);f<<"<svg xmlns='http://www.w3.org/2000/svg' width='"<<w<<"' height='"<<h<<"' viewBox='0 0 "<<w<<" "<<h<<"'>";for(const auto& shape:shapes)f<<shape;f<<"</svg>";}
}
class Adafruit_ST7789 {
 int w=240,h=320,x=0,y=0;unsigned textSize=1;uint16_t textColor=0xFFFF;
 std::string n(int value){return std::to_string(value);}
 void rect(int x,int y,int width,int height,int radius,uint16_t c,bool fill){MockTFT::shapes.push_back("<rect x='"+n(x)+"' y='"+n(y)+"' width='"+n(width)+"' height='"+n(height)+"' rx='"+n(radius)+"' fill='"+(fill?MockTFT::color(c):"none")+"' stroke='"+MockTFT::color(c)+"'/>");}
 public:
 Adafruit_ST7789(SPIClass*,int,int,int){}void init(int,int){}void setRotation(unsigned r){w=r%2?320:240;h=r%2?240:320;}void setTextWrap(bool){}
 int width()const{return w;}int height()const{return h;}
 void fillScreen(uint16_t c){MockTFT::rasterW=w;MockTFT::rasterH=h;MockTFT::pixels.assign(w*h,c);MockTFT::shapes.clear();rect(0,0,w,h,0,c,true);}
 void setTextSize(unsigned s){textSize=s;}void setTextColor(uint16_t c){textColor=c;}void setCursor(int nx,int ny){x=nx;y=ny;}
 void print(const String& s){MockTFT::shapes.push_back("<text x='"+n(x)+"' y='"+n(y+7*textSize)+"' fill='"+MockTFT::color(textColor)+"' font-family='monospace' font-size='"+n(8*textSize)+"' textLength='"+n(s.length()*6*textSize)+"' lengthAdjust='spacingAndGlyphs'>"+MockTFT::escape(s.c_str())+"</text>");}
 void drawLine(int x,int y,int xx,int yy,uint16_t c){MockTFT::shapes.push_back("<path d='M"+n(x)+","+n(y)+" L"+n(xx)+","+n(yy)+"' stroke='"+MockTFT::color(c)+"' fill='none'/>");}
 void drawFastHLine(int x,int y,int len,uint16_t c){drawLine(x,y,x+len,y,c);}
 void fillRect(int x,int y,int w,int h,uint16_t c){MockTFT::directFills.push_back({x,y,w,h});if(!MockTFT::pixels.empty())MockTFT::fill(x,y,w,h,c);rect(x,y,w,h,0,c,true);}void drawRect(int x,int y,int w,int h,uint16_t c){rect(x,y,w,h,0,c,false);}
 void fillRoundRect(int x,int y,int w,int h,int r,uint16_t c){rect(x,y,w,h,r,c,true);}void drawRoundRect(int x,int y,int w,int h,int r,uint16_t c){rect(x,y,w,h,r,c,false);}
 void drawCircle(int x,int y,int r,uint16_t c){MockTFT::shapes.push_back("<circle cx='"+n(x)+"' cy='"+n(y)+"' r='"+n(r)+"' stroke='"+MockTFT::color(c)+"' fill='none'/>");}
 void fillCircle(int x,int y,int r,uint16_t c){MockTFT::shapes.push_back("<circle cx='"+n(x)+"' cy='"+n(y)+"' r='"+n(r)+"' fill='"+MockTFT::color(c)+"'/>");}
 void drawRGBBitmap(int x,int y,uint16_t* bitmap,int width,int height){
  MockTFT::bitmapWrites.push_back({x,y,width,height});
  for(int row=0;row<height;row++)for(int col=0;col<width;col++){int xx=x+col,yy=y+row;if(xx>=0&&yy>=0&&xx<w&&yy<h)MockTFT::pixels[yy*w+xx]=bitmap[row*width+col];}
  // Combine identical adjacent pixel runs vertically for compact SVG previews.
  std::vector<bool> emitted(width*height,false);
  for(int row=0;row<height;row++)for(int col=0;col<width;col++)if(!emitted[row*width+col]){
   uint16_t color=bitmap[row*width+col];int run=1;while(col+run<width&&!emitted[row*width+col+run]&&bitmap[row*width+col+run]==color)run++;
   int rows=1;while(row+rows<height){bool same=true;for(int k=0;k<run;k++)if(emitted[(row+rows)*width+col+k]||bitmap[(row+rows)*width+col+k]!=color)same=false;if(!same)break;rows++;}
   for(int r=0;r<rows;r++)for(int c=0;c<run;c++)emitted[(row+r)*width+col+c]=true;
   rect(x+col,y+row,run,rows,0,color,true);
  }
 }
 void enableDisplay(bool on){MockTFT::enabled=on;}
};
