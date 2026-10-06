#include "MediaDecoder.h"
#include <algorithm>
#include <limits>
#include <new>
namespace Media {
namespace {
class Reader {
  Source& source;Output* control;uint8_t bytes[512]{};uint32_t base=0;size_t valid=0;
public:
  uint32_t pos;bool cancelled=false;
  Reader(Source& s,uint32_t start,Output* out):source(s),control(out),pos(start) {}
  int get() {
    if(pos>=source.size())return -1;
    if(!valid||pos<base||pos-base>=valid) {
      if(control&&!control->check()) { cancelled=true;return -1; }
      if(!source.seek(pos))return -1;
      base=pos;valid=source.read(bytes,std::min<size_t>(512,source.size()-pos));if(!valid)return -1;
    }
    return bytes[pos++-base];
  }
  bool skip(uint32_t n) { if(n>source.size()-pos)return false;pos+=n;return true; }
};
uint16_t le16(const uint8_t* b) { return uint16_t(b[0])|(uint16_t(b[1])<<8); }
uint32_t le32(const uint8_t* b) { return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24); }
}
Result findJPEG(Source& source,uint32_t offset,Frame& frame,std::string& error,Output* control) {
  error.clear();if(offset>=source.size())return Result::End;
  Reader r(source,offset,control);int previous=-1,c=-1;bool found=false,scan=false;
  for(int n=0;n<4096&&(c=r.get())>=0;n++) {
    if(previous==255&&c==216) { frame.begin=r.pos-2;found=true;break; }previous=c;
  }
  if(r.cancelled)return Result::Cancelled;
  if(!found) { if(r.pos==source.size())return Result::End;error="JPEG header not found";return Result::Error; }
  bool entropy=false;
  while(r.pos-frame.begin<=8u*1024*1024) {
    int marker=-1;
    if(entropy) {
      do { c=r.get(); }while(c>=0&&c!=255);
      if(c<0)break;
      do { marker=r.get(); }while(marker==255);
      if(marker==0||(marker>=208&&marker<=215))continue;
      entropy=false;
    } else {
      if(r.get()!=255)break;
      do { marker=r.get(); }while(marker==255);
    }
    if(marker<0)break;
    if(marker==217&&scan) { frame.end=r.pos;return Result::Ok; }
    if(marker==216||marker==217||marker==0||(marker>=208&&marker<=215))break;
    if(marker==1)continue;
    int hi=r.get(),lo=r.get();if(hi<0||lo<0)break;
    uint32_t length=(hi<<8)|lo;
    if(length<2||r.pos-frame.begin>8u*1024*1024||length-2>8u*1024*1024-(r.pos-frame.begin)||!r.skip(length-2))break;
    if(marker==218)entropy=scan=true;
  }
  if(r.cancelled)return Result::Cancelled;
  error="Truncated / invalid JPEG frame (max 8 MiB)";return Result::Error;
}
void Decoder::fit(int w,int h,int boundsW,int boundsH) {
  width=w;height=h;
  if(width>boundsW) { width=boundsW;height=std::max(1,int(int64_t(h)*width/w)); }
  if(height>boundsH) { height=boundsH;width=std::max(1,int(int64_t(w)*height/h)); }
  x=(boundsW-width)/2;y=(boundsH-height)/2;
}
size_t Decoder::input(JDEC* jd,uint8_t* bytes,size_t count) {
  auto& self=*static_cast<Decoder*>(jd->device);
  if(!self.output->check()||self.source->position()>=self.limit)return 0;
  count=std::min<size_t>(count,self.limit-self.source->position());
  if(!bytes)return self.source->seek(self.source->position()+count)?count:0;
  return self.source->read(bytes,count);
}
int Decoder::jpegBlock(JDEC* jd,void* image,JRECT* rect) {
  auto& self=*static_cast<Decoder*>(jd->device);if(!self.output->check())return 0;
  int left=(rect->left*self.width+self.scaledW-1)/self.scaledW;
  int right=((rect->right+1)*self.width+self.scaledW-1)/self.scaledW;
  int top=(rect->top*self.height+self.scaledH-1)/self.scaledH;
  int bottom=((rect->bottom+1)*self.height+self.scaledH-1)/self.scaledH;
  if(right<=left||bottom<=top)return 1;
  auto* rgb=static_cast<uint16_t*>(image);int stride=rect->right-rect->left+1,index=0;
  for(int yy=top;yy<bottom;yy++)for(int xx=left;xx<right;xx++) {
    int sx=xx*self.scaledW/self.width-rect->left,sy=yy*self.scaledH/self.height-rect->top;
    self.pixels[index++]=rgb[sy*stride+sx];
  }
  return self.output->block(self.x+left,self.y+top,self.pixels,right-left,bottom-top)?1:0;
}
Result Decoder::jpeg(Source& in,const Frame& frame,Output& out,int boundsW,int boundsH,int top) {
  error.clear();if(boundsW<1||boundsH<1||frame.end<=frame.begin||frame.end>in.size()||!in.seek(frame.begin)) { error="Invalid JPEG bounds";return Result::Error; }
  if(!workspace)workspace.reset(new(std::nothrow) uint8_t[Workspace]);
  if(!workspace) { error="JPEG workspace unavailable";return Result::Error; }
  source=&in;output=&out;limit=frame.end;JDEC jd{};jd.swap=0;
  JRESULT result=jd_prepare(&jd,input,workspace.get(),Workspace,this);
  if(!out.check())return Result::Cancelled;
  if(result!=JDR_OK) { error=result==JDR_FMT3?"Use baseline RGB JPEG (no progressive / CMYK)":"JPEG header error: "+std::to_string(result);return Result::Error; }
  if(!jd.width||!jd.height||jd.width>4096||jd.height>4096) { error="Image maximum 4096 x 4096";return Result::Error; }
  fit(jd.width,jd.height,boundsW,boundsH);y+=top;
  uint8_t scale=0;
  while(scale<3&&(jd.width>>(scale+1))>=width&&(jd.height>>(scale+1))>=height)scale++;
  scaledW=jd.width>>scale;scaledH=jd.height>>scale;
  out.begin(x,y,width,height);result=jd_decomp(&jd,jpegBlock,scale);
  if(!out.check()||result==JDR_INTR)return Result::Cancelled;
  if(result!=JDR_OK) { error="JPEG data error: "+std::to_string(result);return Result::Error; }
  return Result::Ok;
}
Result Decoder::bmp(Source& in,Output& out,int boundsW,int boundsH,int top) {
  error.clear();uint8_t header[54];
  if(boundsW<1||boundsH<1||!in.seek(0)||in.read(header,54)!=54||header[0]!='B'||header[1]!='M') { error="Invalid BMP header";return Result::Error; }
  uint32_t offset=le32(header+10),dib=le32(header+14);int32_t srcW=le32(header+18),signedH=le32(header+22);
  uint16_t bits=le16(header+28);int64_t srcH=signedH<0?-int64_t(signedH):signedH;
  if(dib<40||srcW<1||srcW>4096||srcH<1||srcH>4096||le16(header+26)!=1||(bits!=24&&bits!=32)||le32(header+30)!=0||uint64_t(offset)<14ull+dib) {
    error="Use uncompressed RGB BMP, 24/32 bit, <=4096 x 4096";return Result::Error;
  }
  int bytesPerPixel=bits/8;uint32_t stride=(uint32_t(srcW)*bytesPerPixel+3)&~3u;
  if(uint64_t(offset)+uint64_t(stride)*srcH>in.size()) { error="Truncated BMP pixels";return Result::Error; }
  fit(srcW,srcH,boundsW,boundsH);y+=top;out.begin(x,y,width,height);
  uint8_t cache[256];
  for(int dy=0;dy<height;dy++) {
    if(!out.check())return Result::Cancelled;
    int sy=int64_t(dy)*srcH/height;if(signedH>0)sy=srcH-1-sy;
    int cached=-1,count=0;
    for(int dx=0;dx<width;) {
      int n=std::min(256,width-dx);
      for(int i=0;i<n;i++) {
        int sx=int64_t(dx+i)*srcW/width;
        if(cached<0||sx>=cached+count) {
          cached=sx;count=std::min(64,int(srcW)-sx);
          if(!in.seek(offset+sy*stride+sx*bytesPerPixel)||in.read(cache,count*bytesPerPixel)!=size_t(count*bytesPerPixel)) { error="BMP read failed";return Result::Error; }
        }
        auto* p=cache+(sx-cached)*bytesPerPixel;
        pixels[i]=((p[2]&248)<<8)|((p[1]&252)<<3)|(p[0]>>3);
      }
      if(!out.block(x+dx,y+dy,pixels,n,1))return Result::Cancelled;
      dx+=n;
    }
  }
  return Result::Ok;
}
Result Movie::next(Source& source,Frame& frame,uint32_t& index,std::string& error,Output* control) {
  Result result=findJPEG(source,nextOffset,frame,error,control);if(result!=Result::Ok)return result;
  index=nextIndex;offsets[index%64]=frame.begin;
  if(index>=first+count) {
    if(index!=first+count) { first=index;count=0; }
    if(count==64)first++;else count++;
  }
  nextIndex++;nextOffset=frame.end;return Result::Ok;
}
void Movie::seekFrames(int delta) {
  target=uint32_t(std::max<int64_t>(0,presented+delta));
  if(target>=first&&target<first+count) { nextIndex=target;nextOffset=offsets[target%64]; }
  else if(target>=nextIndex) { /* Continue scanning forward without decoding skipped frames. */ }
  else { nextIndex=0;nextOffset=0;first=count=0; }
}
}
