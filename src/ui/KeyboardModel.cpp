#include "KeyboardModel.h"
#include <algorithm>
namespace {
constexpr char symbols[]="0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
}
void KeyboardModel::begin(const char* name,const char* initial,size_t maximum,bool secret,int mode) {
  title=name; value=initial; limit=maximum; password=secret; selected=0;
  language=mode==2?Language::Symbols:Language::English;
  shift=mode==1||mode==3;
  while(value.size()>limit)erase();
}
int KeyboardModel::characters()const {
  return language==Language::English?27:language==Language::Russian?34:sizeof(symbols)-1;
}
uint32_t KeyboardModel::character(int index)const {
  if(index<0||index>=characters())return 0;
  if(language==Language::Symbols)return symbols[index];
  if(index==characters()-1)return ' ';
  if(language==Language::English)return (shift?'A':'a')+index;
  // Russian alphabet includes ё after е.
  uint32_t cp=index==6?0x451:0x430+index-(index>6?1:0);
  return shift?(cp==0x451?0x401:cp-32):cp;
}
void KeyboardModel::move(Direction direction) {
  int chars=characters(), row=selected<chars?selected/9:rows();
  int column=selected<chars?selected%9:selected-chars;
  auto width=[this,chars](int r) { return r==rows()?4:std::min(9,chars-r*9); };
  if(direction==Direction::Left||direction==Direction::Right) {
    int n=width(row);
    column=(column+(direction==Direction::Left?n-1:1))%n;
  } else {
    int oldColumns=row==rows()?4:9;
    int target=(row+(direction==Direction::Up?rows():1))%(rows()+1);
    int newColumns=target==rows()?4:9;
    column=std::min(width(target)-1,((column*2+1)*newColumns)/(oldColumns*2));
    row=target;
  }
  selected=row==rows()?chars+column:row*9+column;
}
bool KeyboardModel::press() {
  int chars=characters();
  if(selected<chars) {
    auto letter=utf8(character(selected));
    if(value.size()+letter.size()<=limit)value+=letter;
    return false;
  }
  int control=selected-chars;
  if(control==Enter)return true;
  if(control==Lang)language=(Language)(((int)language+1)%3);
  else if(control==Backspace)erase();
  else if(control==Shift)shift=!shift;
  selected=characters()+control;
  return false;
}
void KeyboardModel::erase() {
  if(value.empty())return;
  size_t start=value.size()-1;
  while(start && ((uint8_t)value[start]&0xC0)==0x80)start--;
  value.erase(start);
}
std::string KeyboardModel::utf8(uint32_t cp) {
  if(cp<128)return std::string(1,(char)cp);
  if(cp<2048)return {char(0xC0|(cp>>6)),char(0x80|(cp&63))};
  return {char(0xE0|(cp>>12)),char(0x80|((cp>>6)&63)),char(0x80|(cp&63))};
}
uint32_t KeyboardModel::next(const std::string& text,size_t& offset) {
  if(offset>=text.size())return 0;
  uint8_t first=text[offset++];
  if(first<128)return first;
  int continuation=(first&0xE0)==0xC0?1:(first&0xF0)==0xE0?2:(first&0xF8)==0xF0?3:0;
  if(!continuation)return '?';
  uint32_t cp=first&((1<<(6-continuation))-1);
  for(int i=0;i<continuation;i++) {
    if(offset>=text.size()||((uint8_t)text[offset]&0xC0)!=0x80)return '?';
    cp=(cp<<6)|((uint8_t)text[offset++]&63);
  }
  return cp;
}
size_t KeyboardModel::count(const std::string& text) {
  size_t offset=0,n=0;
  while(offset<text.size()) { next(text,offset); n++; }
  return n;
}
