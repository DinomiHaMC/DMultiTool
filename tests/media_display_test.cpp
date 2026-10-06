#include "../src/media/MediaDecoder.h"
#include "../src/ui/DisplayManager.h"
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <iostream>
struct Source:Media::Source {
 std::vector<uint8_t> bytes;uint32_t pos=0;
 explicit Source(const char* path){std::ifstream f(path,std::ios::binary);assert(f);bytes.assign(std::istreambuf_iterator<char>(f),{});}
 uint32_t size()const override{return bytes.size();}uint32_t position()const override{return pos;}
 bool seek(uint32_t n)override{if(n>size())return false;pos=n;return true;}
 size_t read(uint8_t* out,size_t n)override{n=std::min<size_t>(n,size()-pos);std::copy_n(bytes.data()+pos,n,out);pos+=n;return n;}
};
struct Output:Media::Output {
 DisplayManager& display;explicit Output(DisplayManager& d):display(d){}
 bool check()override{return true;}
 void begin(int x,int y,int w,int h)override{display.beginMedia(x,y,w,h);}
 bool block(int x,int y,uint16_t* p,int w,int h)override{display.mediaBlock(x,y,p,w,h);return true;}
};
int main(){
 DisplayManager display;display.begin(0);Output output(display);Media::Decoder decoder;Source source("examples/sd/media/photo.jpg");Media::Frame frame;std::string error;
 assert(Media::findJPEG(source,0,frame,error)==Media::Result::Ok);
 for(int rotation=0;rotation<2;rotation++){
  display.rotation(rotation);display.invalidate();assert(display.mediaNeedsRedraw());
  assert(decoder.jpeg(source,frame,output,display.width(),display.height()-52)==Media::Result::Ok);
  assert(!display.mediaNeedsRedraw());display.mediaStatus("photo.jpg","LR files | UD rotate | Hold OK exit");
  MockTFT::write(rotation?"docs/screenshots/media-landscape.svg":"docs/screenshots/media-photo.svg",display.width(),display.height());
  auto before=MockTFT::pixels;size_t shapes=MockTFT::shapes.size();
  display.mediaStatus("photo.jpg","LR files | UD rotate | Hold OK exit");assert(shapes==MockTFT::shapes.size());
  MockTFT::directFills.clear();assert(decoder.jpeg(source,frame,output,display.width(),display.height()-52)==Media::Result::Ok);
  assert(MockTFT::pixels==before&&MockTFT::directFills.empty());
  for(auto r:MockTFT::bitmapWrites)assert(r.y>=26&&r.y+r.h<=display.height()-26);
  MockTFT::bitmapWrites.clear();display.invalidate();assert(display.mediaNeedsRedraw());
 }
 Source video("examples/sd/media/demo.mjpeg");Media::Movie movie;uint32_t index;
 display.rotation(0);display.invalidate();assert(movie.next(video,frame,index,error)==Media::Result::Ok);
 assert(decoder.jpeg(video,frame,output,display.width(),display.height()-52)==Media::Result::Ok);
 display.mediaStatus("demo.mjpeg","PLAY 10fps #1 | Hold OK exit");
 MockTFT::write("docs/screenshots/media-video.svg",240,320);
 std::cout<<"Media display passed: actual JPEG output, both orientations, viewport, HUD, redraw without clearing\n";
}
