#pragma once
#include <SD.h>
#include "../config.h"
struct FileEntry  {
  char name[256];
  uint64_t size;
  bool directory;
};
class SDModule  {
  public: bool mounted=false, truncated=false;
  FileEntry entries[Config::MaxEntries];
  size_t count=0;
  void begin();
  bool list(const String& path,uint32_t offset=0);
  bool removeFile(const String& path);
  bool writeNew(const String& path,const String& content);
  bool readPage(const String& path,uint32_t offset,String& text,uint32_t& next);
};
