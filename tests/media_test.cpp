#include "../src/media/MediaDecoder.h"
#include <cassert>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>
#include <iostream>
#include <cmath>
using namespace Media;
struct Memory:Source {
  std::vector<uint8_t> data;uint32_t offset=0;bool fail=false;
  uint32_t size()const override { return data.size(); }
  uint32_t position()const override { return offset; }
  bool seek(uint32_t pos)override { if(pos>data.size())return false;offset=pos;return true; }
  size_t read(uint8_t* bytes,size_t n)override { if(fail)return 0;n=std::min<size_t>(n,data.size()-offset);std::copy_n(data.data()+offset,n,bytes);offset+=n;return n; }
};
Memory load(const char* name) {
  Memory m;std::ifstream file(std::string("tests/media-fixtures/")+name,std::ios::binary);assert(file);
  m.data.assign(std::istreambuf_iterator<char>(file),{});return m;
}
struct Raster:Output {
  int boundsW=240,boundsH=268,x=0,y=0,w=0,h=0,checks=0,maxChecks=100000;
  std::vector<uint16_t> pixels;std::vector<int> written;
  bool check()override { return ++checks<=maxChecks; }
  void begin(int xx,int yy,int width,int height)override { x=xx;y=yy;w=width;h=height;assert(x>=0&&y>=26&&x+w<=boundsW&&y+h<=boundsH+26);pixels.assign(w*h,0);written.assign(w*h,0); }
  bool block(int xx,int yy,uint16_t* rgb,int width,int height)override {
    assert(width*height<=256&&xx>=x&&yy>=y&&xx+width<=x+w&&yy+height<=y+h);
    for(int row=0;row<height;row++)for(int col=0;col<width;col++) { int at=(yy-y+row)*w+xx-x+col;pixels[at]=rgb[row*width+col];written[at]++; }
    return check();
  }
  void complete()const { assert(!written.empty());for(int n:written)assert(n==1); }
};
void put16(std::vector<uint8_t>& b,int at,uint16_t value){b[at]=value&255;b[at+1]=value>>8;}
void put32(std::vector<uint8_t>& b,int at,uint32_t value){for(int i=0;i<4;i++)b[at+i]=value>>(i*8);}
Memory bmp(int width,int height,int bits,bool topDown) {
  Memory m;int stride=(width*(bits/8)+3)&~3;m.data.resize(54+stride*height);
  m.data[0]='B';m.data[1]='M';put32(m.data,2,m.data.size());put32(m.data,10,54);put32(m.data,14,40);put32(m.data,18,width);put32(m.data,22,topDown?-height:height);put16(m.data,26,1);put16(m.data,28,bits);
  for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
    int row=topDown?y:height-1-y,at=54+row*stride+x*(bits/8);m.data[at]=x*8;m.data[at+1]=y*24;m.data[at+2]=255-x*8;
  }
  return m;
}
int main() {
  Decoder decoder;Frame frame;std::string error;
  for(const char* name:{"corners.jpg","metadata.jpg","grayscale.jpg","wide.jpg","odd.jpg"}) {
    Memory image=load(name);assert(findJPEG(image,0,frame,error)==Result::Ok&&frame.end==image.size());
    for(auto size:{std::pair<int,int>{240,268},{320,188}}) {
      Raster raster;raster.boundsW=size.first;raster.boundsH=size.second;
      assert(decoder.jpeg(image,frame,raster,raster.boundsW,raster.boundsH)==Result::Ok);raster.complete();
      if(std::string(name)=="corners.jpg") {
        assert(raster.w==64&&raster.h==48);
        assert((raster.pixels[12*64+16]&0xf800)>=0xf000);
        assert((raster.pixels[12*64+48]&0x07e0)>=0x07a0);
        assert((raster.pixels[36*64+16]&0x001f)>=0x001d);
      }
    }
  }
  for(const char* name:{"progressive.jpg","cmyk.jpg"}) { auto m=load(name);Raster out;assert(findJPEG(m,0,frame,error)==Result::Ok);assert(decoder.jpeg(m,frame,out,240,268)==Result::Error);assert(!decoder.error.empty()); }
  auto jpeg=load("corners.jpg");Raster abort;abort.maxChecks=1;assert(findJPEG(jpeg,0,frame,error)==Result::Ok);assert(decoder.jpeg(jpeg,frame,abort,240,268)==Result::Cancelled);
  abort.checks=0;abort.maxChecks=0;assert(findJPEG(jpeg,0,frame,error,&abort)==Result::Cancelled);
  auto broken=jpeg;broken.data.resize(broken.data.size()-10);assert(findJPEG(broken,0,frame,error)==Result::Error);
  broken=jpeg;broken.data[4]=0;broken.data[5]=1;assert(findJPEG(broken,0,frame,error)==Result::Error);
  broken=jpeg;broken.fail=true;assert(findJPEG(broken,0,frame,error)==Result::Error);
  Memory video=load("two_frames.mjpeg");Movie movie;uint32_t index;
  assert(movie.next(video,frame,index,error)==Result::Ok&&index==0);uint32_t first=frame.begin;movie.presented=0;
  assert(movie.next(video,frame,index,error)==Result::Ok&&index==1);movie.presented=1;
  assert(movie.next(video,frame,index,error)==Result::End);movie.seekFrames(-1);
  assert(movie.next(video,frame,index,error)==Result::Ok&&index==0&&frame.begin==first);
  Memory many;for(int i=0;i<90;i++)many.data.insert(many.data.end(),jpeg.data.begin(),jpeg.data.end());movie.reset();
  for(int i=0;i<90;i++) { assert(movie.next(many,frame,index,error)==Result::Ok&&index==uint32_t(i));movie.presented=i; }
  movie.seekFrames(-1);assert(movie.next(many,frame,index,error)==Result::Ok&&index==88);
  movie.presented=88;movie.seekFrames(-80);assert(movie.skipping());
  while(movie.skipping())assert(movie.next(many,frame,index,error)==Result::Ok);
  assert(movie.next(many,frame,index,error)==Result::Ok&&index==8);
  movie.presented=8;movie.seekFrames(15);while(movie.skipping())assert(movie.next(many,frame,index,error)==Result::Ok);
  assert(movie.next(many,frame,index,error)==Result::Ok&&index==23);
  std::vector<uint16_t> reference;
  for(int bits:{24,32})for(bool topDown:{false,true}) {
    Memory image=bmp(7,5,bits,topDown);Raster raster;assert(decoder.bmp(image,raster,240,268)==Result::Ok);raster.complete();
    if(reference.empty())reference=raster.pixels;else assert(reference==raster.pixels);
    image.data.resize(image.data.size()-1);assert(decoder.bmp(image,raster,240,268)==Result::Error);
  }
  auto large=bmp(401,257,24,false);Raster out;assert(decoder.bmp(large,out,240,268)==Result::Ok);out.complete();assert(out.w==240&&out.h<268);
  auto bad=bmp(7,5,24,false);put32(bad.data,30,1);assert(decoder.bmp(bad,out,240,268)==Result::Error);
  bad=bmp(7,5,24,false);put32(bad.data,18,0xffffffff);assert(decoder.bmp(bad,out,240,268)==Result::Error);
  bad=bmp(7,5,24,false);put32(bad.data,22,0x80000000);assert(decoder.bmp(bad,out,240,268)==Result::Error);
  bad=bmp(7,5,24,false);put32(bad.data,10,0xfffffff0);assert(decoder.bmp(bad,out,240,268)==Result::Error);
  out.checks=0;out.maxChecks=1;assert(decoder.bmp(large,out,240,268)==Result::Cancelled);
  // Bounded corrupt-frame scanner: no allocation or unbounded loops on malformed markers.
  uint32_t seed=1;
  for(int n=0;n<500;n++) {
    Memory fuzz=jpeg;for(int i=0;i<4;i++){seed=seed*1664525+1013904223;fuzz.data[(seed>>8)%fuzz.data.size()]=seed>>24;}
    Result result=findJPEG(fuzz,0,frame,error);
    if(result==Result::Ok){Raster check;check.maxChecks=10000;decoder.jpeg(fuzz,frame,check,240,268);}
  }
  decoder.close();std::cout<<"Media tests passed: real baseline JPEG/color/gray/scaling/metadata, progressive/CMYK rejection, BMP orientation/padding/32bit/bounds, MJPEG framing/EOF/seek/cache rollover, cancellation, corrupt files\n";
}
