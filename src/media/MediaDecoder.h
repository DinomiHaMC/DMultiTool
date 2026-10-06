#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <array>
#include "../third_party/tjpgd/tjpgd.h"
namespace Media {
enum class Result { Ok, End, Error, Cancelled };
struct Frame { uint32_t begin=0,end=0; };
class Source {
public:
  virtual ~Source()=default;
  virtual uint32_t size()const=0;
  virtual uint32_t position()const=0;
  virtual bool seek(uint32_t offset)=0;
  virtual size_t read(uint8_t* bytes,size_t count)=0;
};
class Output {
public:
  virtual ~Output()=default;
  virtual bool check()=0;
  virtual void begin(int x,int y,int width,int height)=0;
  virtual bool block(int x,int y,uint16_t* pixels,int width,int height)=0;
};
Result findJPEG(Source& source,uint32_t offset,Frame& frame,std::string& error,Output* control=nullptr);
class Decoder {
  std::unique_ptr<uint8_t[]> workspace;
  Source* source=nullptr;Output* output=nullptr;
  uint32_t limit=0;int x=0,y=0,width=0,height=0,scaledW=0,scaledH=0;
  uint16_t pixels[256]{};
  static size_t input(JDEC* jd,uint8_t* bytes,size_t count);
  static int jpegBlock(JDEC* jd,void* pixels,JRECT* rect);
  void fit(int w,int h,int boundsW,int boundsH);
public:
  static constexpr int Workspace=6144;
  std::string error;
  Result jpeg(Source& source,const Frame& frame,Output& output,int boundsW,int boundsH,int top=26);
  Result bmp(Source& source,Output& output,int boundsW,int boundsH,int top=26);
  void close() { workspace.reset();source=nullptr;output=nullptr; }
};
class Movie {
  std::array<uint32_t,64> offsets{};
  uint32_t first=0,count=0;
public:
  uint32_t nextOffset=0,nextIndex=0,target=0;
  int64_t presented=-1;
  void reset() { nextOffset=nextIndex=target=first=count=0;presented=-1; }
  Result next(Source& source,Frame& frame,uint32_t& index,std::string& error,Output* control=nullptr);
  bool skipping()const { return nextIndex<target; }
  void seekFrames(int delta);
};
}
