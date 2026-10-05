#pragma once
#include <cstdint>
#include <string>

// Independent of menu rows and hardware: one full-screen input widget.
class KeyboardModel {
public:
  enum class Language { English, Russian, Symbols };
  enum class Direction { Up, Down, Left, Right };
  enum Control { Lang, Enter, Backspace, Shift };
  std::string title, value;
  Language language=Language::English;
  bool shift=false, password=false;
  int selected=0;
  size_t limit=128; // Bytes, preserving existing network/file/protocol bounds.
  void begin(const char* name,const char* initial,size_t maximum,bool secret,int mode);
  int characters()const;
  int rows()const { return (characters()+8)/9; }
  uint32_t character(int index)const;
  void move(Direction direction);
  bool press(); // true only for Enter
  void erase();
  static std::string utf8(uint32_t codepoint);
  static uint32_t next(const std::string& text,size_t& offset);
  static size_t count(const std::string& text);
};
