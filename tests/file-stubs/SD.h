#pragma once
#include <Arduino.h>
#include <map>
#include <memory>
#include <vector>
#include <cstring>
#define FILE_WRITE "w"
struct Node { std::vector<uint8_t> bytes;bool directory=false; };
inline bool failWrites=false;
class File {
  std::shared_ptr<Node> node;uint32_t offset=0;
public:
  File()=default;explicit File(std::shared_ptr<Node> n):node(std::move(n)) {}
  explicit operator bool()const { return bool(node); }
  bool isDirectory()const { return node&&node->directory; }
  uint32_t size()const { return node?node->bytes.size():0; }
  uint32_t available()const { return size()-offset; }
  uint32_t position()const { return offset; }
  bool seek(uint32_t value) { if(value>size())return false;offset=value;return true; }
  int read(uint8_t* bytes,size_t count) { count=std::min(count,size_t(available()));std::memcpy(bytes,node->bytes.data()+offset,count);offset+=count;return count; }
  size_t write(const uint8_t* bytes,size_t count) {
    if(!node||node->directory||failWrites)return 0;
    if(offset+count>node->bytes.size())node->bytes.resize(offset+count);
    std::memcpy(node->bytes.data()+offset,bytes,count);offset+=count;return count;
  }
  void flush() {}void close() { node.reset(); }
  String readString()const { return node?String(std::string(node->bytes.begin(),node->bytes.end())):String(); }
};
class FakeFS {
public:
  std::map<std::string,std::shared_ptr<Node>> nodes;
  bool exists(const String& path) { return nodes.count(path.c_str()); }
  File open(const String& path,const char* mode="r") {
    std::string key=path.c_str();
    if(mode[0]=='w'&&!nodes.count(key))nodes[key]=std::make_shared<Node>();
    return nodes.count(key)?File(nodes[key]):File();
  }
  bool remove(const String& path) { return nodes.erase(path.c_str())!=0; }
  bool rename(const String& from,const String& to) {
    if(!exists(from)||exists(to))return false;
    nodes[to.c_str()]=nodes[from.c_str()];nodes.erase(from.c_str());return true;
  }
};
inline FakeFS SD;
